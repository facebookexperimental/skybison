#include "builtins.h"
#include "dict-builtins.h"
#include "handles.h"
#include "objects.h"
#include "runtime.h"
#include "str-builtins.h"
#include "thread.h"
#include "unicode.h"
#include "utils.h"

namespace py {

static const int kNumUEscapeChars = 4;

enum class LoadsArg {
  kString = 0,
  kEncoding = 1,
  kCls = 2,
  kObjectHook = 3,
  kParseFloat = 4,
  kParseInt = 5,
  kParseConstant = 6,
  kObjectPairsHook = 7,
  kKw = 8,
};

struct JSONParser {
  // Index of next byte to read.
  word next;
  word length;
  Arguments args;
  bool has_parse_constant;
  bool has_parse_int;
  bool strict;
};

static NEVER_INLINE int callParseConstant(Thread* thread, JSONParser* env,
                                          const DataArray& data, word length,
                                          Object* value_out) {
  HandleScope scope(thread);
  Object hook(&scope,
              env->args.get(static_cast<word>(LoadsArg::kParseConstant)));
  Str string(&scope, dataArraySubstr(thread, data, env->next - length, length));
  *value_out = Interpreter::call1(thread, hook, string);
  if (value_out->isErrorException()) return -1;
  return 0;
}

static NEVER_INLINE RawObject callParseInt(Thread* thread, JSONParser* env,
                                           const DataArray& data, word begin) {
  HandleScope scope(thread);
  Object hook(&scope, env->args.get(static_cast<word>(LoadsArg::kParseInt)));
  Object str(&scope, dataArraySubstr(thread, data, begin, env->next - begin));
  return Interpreter::call1(thread, hook, str);
}

static byte nextNonWhitespace(Thread*, JSONParser* env, const DataArray& data) {
  word next = env->next;
  word length = env->length;
  byte b;
  do {
    if (next >= length) {
      // Set `next` to `length + 1` to indicate EOF (end of file).
      env->next = length + 1;
      return 0;
    }
    b = data.byteAt(next++);
  } while (b == ' ' || b == '\t' || b == '\n' || b == '\r');
  env->next = next;
  return b;
}

static NEVER_INLINE RawObject raiseJSONDecodeError(Thread* thread,
                                                   JSONParser* env,
                                                   const DataArray& data,
                                                   word index,
                                                   const char* msg) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object json_decode_error(&scope, runtime->lookupNameInModule(
                                       thread, ID(_json), ID(JSONDecodeError)));
  CHECK(json_decode_error.isType(), "_json.JSONDecodeError not found");

  // TODO(T81331502): Add helper function for byte offset to code point index
  // translation.
  word pos = 0;
  for (word i = 0, cp_length; i < index; i += cp_length) {
    data.codePointAt(i, &cp_length);
    pos++;
  }

  // Convert byte position to codepoint.
  Object msg_str(&scope, runtime->newStrFromCStr(msg));
  Object doc(&scope, env->args.get(static_cast<word>(LoadsArg::kString)));
  Object pos_obj(&scope, runtime->newInt(pos));
  Object args(&scope, runtime->newTupleWith3(msg_str, doc, pos_obj));
  return thread->raiseWithType(*json_decode_error, *args);
}

static RawObject scanEscapeSequence(Thread* thread, JSONParser* env,
                                    const DataArray& data, word begin) {
  word next = env->next;
  word length = env->length;
  if (next >= length) {
    return raiseJSONDecodeError(thread, env, data, begin - 1,
                                "Unterminated string starting at");
  }
  byte ascii_result;
  byte b = data.byteAt(next++);
  switch (b) {
    case '"':
    case '\\':
    case '/':
      ascii_result = b;
      break;
    case 'b':
      ascii_result = '\b';
      break;
    case 'f':
      ascii_result = '\f';
      break;
    case 'n':
      ascii_result = '\n';
      break;
    case 'r':
      ascii_result = '\r';
      break;
    case 't':
      ascii_result = '\t';
      break;
    case 'u': {
      int32_t code_point;
      if (next >= length - kNumUEscapeChars) {
        return raiseJSONDecodeError(thread, env, data, next - 1,
                                    "Invalid \\uXXXX escape");
      }
      code_point = 0;
      word end = next + kNumUEscapeChars;
      do {
        b = data.byteAt(next++);
        code_point <<= kBitsPerHexDigit;
        if ('0' <= b && b <= '9') {
          code_point |= b - '0';
        } else if ('a' <= b && b <= 'f') {
          code_point |= b - 'a' + 10;
        } else if ('A' <= b && b <= 'F') {
          code_point |= b - 'A' + 10;
        } else {
          return raiseJSONDecodeError(thread, env, data, end - kNumUEscapeChars,
                                      "Invalid \\uXXXX escape");
        }
      } while (next < end);
      if (Unicode::isHighSurrogate(code_point) &&
          next < length - (kNumUEscapeChars + 2) && data.byteAt(next) == '\\' &&
          data.byteAt(next + 1) == 'u') {
        word next2 = next + 2;
        int32_t code_point2 = 0;
        word end2 = next2 + kNumUEscapeChars;
        do {
          byte b2 = data.byteAt(next2++);
          code_point2 <<= kBitsPerHexDigit;
          if ('0' <= b2 && b2 <= '9') {
            code_point2 |= b2 - '0';
          } else if ('a' <= b2 && b2 <= 'f') {
            code_point2 |= b2 - 'a' + 10;
          } else if ('A' <= b2 && b2 <= 'F') {
            code_point2 |= b2 - 'A' + 10;
          } else {
            code_point2 = 0;
            break;
          }
        } while (next2 < end2);
        if (Unicode::isLowSurrogate(code_point2)) {
          code_point = Unicode::combineSurrogates(code_point, code_point2);
          next = end2;
        }
      }
      env->next = next;
      return SmallStr::fromCodePoint(code_point);
    }
    default:
      return raiseJSONDecodeError(thread, env, data, next - 2,
                                  "Invalid \\escape");
  }
  env->next = next;
  return SmallStr::fromCodePoint(ascii_result);
}

static RawObject scanFloat(Thread*, JSONParser*, const DataArray&, byte, word) {
  UNIMPLEMENTED("float parsing");
}

static RawObject scanLargeInt(Thread* thread, JSONParser* env,
                              const DataArray& data, byte b, word begin,
                              bool negative, word value) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  word next = env->next;
  word length = env->length;
  Int result(&scope, SmallInt::fromWord(value));
  Int factor(&scope, SmallInt::fromWord(SmallInt::kMaxDigits10Pow));
  Int value_int(&scope, SmallInt::fromWord(0));

  value = 0;
  word digits = 0;
  for (;;) {
    value += b - '0';
    if (next >= length) break;
    b = data.byteAt(next++);
    if ('0' <= b && b <= '9') {
      digits++;
      if (digits >= SmallInt::kMaxDigits10) {
        value_int = Int::cast(SmallInt::fromWord(value));
        result = runtime->intMultiply(thread, result, factor);
        result = runtime->intAdd(thread, result, value_int);
        digits = 0;
        value = 0;
      } else {
        value *= 10;
      }
      continue;
    }

    if (b == '.' || b == 'e' || b == 'E') {
      env->next = next;
      return scanFloat(thread, env, data, b, begin);
    }

    next--;
    break;
  }
  env->next = next;
  if (env->has_parse_int) {
    return callParseInt(thread, env, data, begin);
  }

  word f = negative ? -10 : 10;
  for (word i = 0; i < digits; i++) {
    f *= 10;
  }
  factor = Int::cast(SmallInt::fromWord(f));
  result = runtime->intMultiply(thread, result, factor);
  value_int = Int::cast(SmallInt::fromWord(value));
  if (negative) {
    result = runtime->intSubtract(thread, result, value_int);
  } else {
    result = runtime->intAdd(thread, result, value_int);
  }
  return *result;
}

static RawObject scanString(Thread* thread, JSONParser* env,
                            const DataArray& data) {
  struct Segment {
    int32_t begin_or_negative_length;
    int32_t length_or_utf8;
  };

  Runtime* runtime = thread->runtime();
  word next = env->next;
  word length = env->length;
  word result_length = 0;
  Vector<Segment> segments;
  word begin = next;
  word segment_begin;
  word segment_length;
  for (;;) {
    segment_begin = next;
    byte b;
    for (;;) {
      if (next >= length) {
        return raiseJSONDecodeError(thread, env, data, begin - 1,
                                    "Unterminated string starting at");
      }
      b = data.byteAt(next++);
      if (b == '"' || b == '\\') {
        break;
      }
      if (ASCII::isControlCharacter(b) && env->strict) {
        return raiseJSONDecodeError(thread, env, data, next - 1,
                                    "Invalid control character at");
      }
    }
    // Segment ends before the current `"` or `\` character.
    segment_length = next - segment_begin - 1;
    if (b == '"') {
      break;
    }

    if (segment_length > 0) {
      segments.push_back(Segment{static_cast<int32_t>(segment_begin),
                                 static_cast<int32_t>(segment_length)});
      result_length += segment_length;
    }

    DCHECK(b == '\\', "Expected backslash");
    env->next = next;
    RawObject escape_result = scanEscapeSequence(thread, env, data, begin);
    if (escape_result.isErrorException()) return escape_result;
    next = env->next;
    RawSmallStr str = SmallStr::cast(escape_result);
    word str_length = str.length();
    Segment segment;
    segment.begin_or_negative_length = -str_length;
    segment.length_or_utf8 = 0;
    CHECK(str_length <= static_cast<word>(sizeof(segment.length_or_utf8)),
          "encoded codepoint should fit in `length_or_utf8`");
    str.copyTo(reinterpret_cast<byte*>(&segment.length_or_utf8), str_length);
    result_length += str_length;
    segments.push_back(segment);
  }
  env->next = next;
  if (segments.size() == 0) {
    return dataArraySubstr(thread, data, segment_begin, segment_length);
  }
  if (segment_length > 0) {
    segments.push_back(Segment{static_cast<int32_t>(segment_begin),
                               static_cast<int32_t>(segment_length)});
    result_length += segment_length;
  }
  HandleScope scope(thread);
  MutableBytes result(&scope,
                      runtime->newMutableBytesUninitialized(result_length));
  word result_index = 0;
  for (Segment segment : segments) {
    word begin_or_negative_length = segment.begin_or_negative_length;
    word length_or_utf8 = segment.length_or_utf8;
    if (begin_or_negative_length >= 0) {
      result.replaceFromWithStartAt(result_index, *data, length_or_utf8,
                                    begin_or_negative_length);
      result_index += length_or_utf8;
    } else {
      word utf8_length = -begin_or_negative_length;
      result.replaceFromWithAll(
          result_index,
          View<byte>(reinterpret_cast<byte*>(&length_or_utf8), utf8_length));
      result_index += utf8_length;
    }
  }
  DCHECK(result_index == result_length, "index/length mismatch");
  return result.becomeStr();
}

static RawObject scanNumber(Thread* thread, JSONParser* env,
                            const DataArray& data, byte b) {
  word begin = env->next - 1;
  word next = env->next;
  word length = env->length;
  bool negative = (b == '-');
  if (negative) {
    if (next >= length) return Error::error();
    negative = true;
    b = data.byteAt(next++);
    if (b < '0' || b > '9') {
      return Error::error();
    }
  }
  if (b == '0') {
    if (next < length) {
      b = data.byteAt(next++);
      if (b == '.' || b == 'e' || b == 'E') {
        env->next = next;
        return scanFloat(thread, env, data, b, begin);
      }
      next--;
    }
    env->next = next;
    if (env->has_parse_int) {
      return callParseInt(thread, env, data, begin);
    }
    return SmallInt::fromWord(0);
  }

  word value = 0;
  word digits_left = SmallInt::kMaxDigits10;
  for (;;) {
    value += b - '0';
    if (next >= length) break;
    b = data.byteAt(next++);
    if ('0' <= b && b <= '9') {
      digits_left--;
      if (digits_left == 0) {
        env->next = next;
        return scanLargeInt(thread, env, data, b, begin, negative, value);
      }
      value *= 10;
      continue;
    }

    if (b == '.' || b == 'e' || b == 'E') {
      env->next = next;
      return scanFloat(thread, env, data, b, begin);
    }

    next--;
    break;
  }
  env->next = next;
  if (env->has_parse_int) {
    return callParseInt(thread, env, data, begin);
  }
  return SmallInt::fromWord(negative ? -value : value);
}

static int scan(Thread* thread, JSONParser* env, const DataArray& data, byte b,
                Object* value_out) {
  for (;;) {
    word next = env->next;
    word length = env->length;

    switch (b) {
      case '"': {
        *value_out = scanString(thread, env, data);
        if (value_out->isErrorException()) return -1;
        return 0;
      }
      case '{':
        return '{';
      case '[':
        return '[';

      case '-':  // `-Infinity` or number
        if (next <= length - 8 && data.byteAt(next) == 'I' &&
            data.byteAt(next + 1) == 'n' && data.byteAt(next + 2) == 'f' &&
            data.byteAt(next + 3) == 'i' && data.byteAt(next + 4) == 'n' &&
            data.byteAt(next + 5) == 'i' && data.byteAt(next + 6) == 't' &&
            data.byteAt(next + 7) == 'y') {
          env->next = next + 8;
          if (env->has_parse_constant) {
            return callParseConstant(thread, env, data, 9, value_out);
          }
          *value_out = thread->runtime()->newFloat(-kDoubleInfinity);
          return 0;
        }
        FALLTHROUGH;
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9': {
        RawObject value = scanNumber(thread, env, data, b);
        if (value.isError()) {
          if (value.isErrorException()) {
            *value_out = Error::exception();
          } else {
            *value_out = raiseJSONDecodeError(thread, env, data, next - 1,
                                              "Expecting value");
          }
          return -1;
        }
        *value_out = value;
        return 0;
      }

      case 'n':  // `null`
        if (next <= length - 3 && data.byteAt(next) == 'u' &&
            data.byteAt(next + 1) == 'l' && data.byteAt(next + 2) == 'l') {
          env->next = next + 3;
          *value_out = NoneType::object();
          return 0;
        }
        break;
      case 't':  // `true`
        if (next <= length - 3 && data.byteAt(next) == 'r' &&
            data.byteAt(next + 1) == 'u' && data.byteAt(next + 2) == 'e') {
          env->next = next + 3;
          *value_out = Bool::trueObj();
          return 0;
        }
        break;
      case 'f':  // `false`
        if (next <= length - 4 && data.byteAt(next) == 'a' &&
            data.byteAt(next + 1) == 'l' && data.byteAt(next + 2) == 's' &&
            data.byteAt(next + 3) == 'e') {
          env->next = next + 4;
          *value_out = Bool::falseObj();
          return 0;
        }
        break;
      case 'N':  // `NaN`
        if (next <= length - 2 && data.byteAt(next) == 'a' &&
            data.byteAt(next + 1) == 'N') {
          env->next = next + 2;
          if (env->has_parse_constant) {
            return callParseConstant(thread, env, data, 3, value_out);
          }
          *value_out = thread->runtime()->newFloat(kDoubleNaN);
          return 0;
        }
        break;
      case 'I':  // `Infinity`
        if (next <= length - 7 && data.byteAt(next) == 'n' &&
            data.byteAt(next + 1) == 'f' && data.byteAt(next + 2) == 'i' &&
            data.byteAt(next + 3) == 'n' && data.byteAt(next + 4) == 'i' &&
            data.byteAt(next + 5) == 't' && data.byteAt(next + 6) == 'y') {
          env->next = next + 7;
          if (env->has_parse_constant) {
            return callParseConstant(thread, env, data, 8, value_out);
          }
          *value_out = thread->runtime()->newFloat(kDoubleInfinity);
          return 0;
        }
        break;
      default:
        break;
    }
    DCHECK(b != ' ' && b != '\t' && b != '\r' && b != '\n',
           "whitespace not skipped");
    *value_out =
        raiseJSONDecodeError(thread, env, data, next - 1, "Expecting value");
    return -1;
  }
}

static RawObject parse(Thread* thread, JSONParser* env, const DataArray& data) {
  HandleScope scope(thread);

  Object value(&scope, NoneType::object());
  byte b = nextNonWhitespace(thread, env, data);
  int scan_result = scan(thread, env, data, b, &value);
  switch (scan_result) {
    case 0:
      // Already have a finished object.
      b = nextNonWhitespace(thread, env, data);
      break;
    case '[':
      UNIMPLEMENTED("lists");
    case '{':
      UNIMPLEMENTED("dictionaries");
    default:
      DCHECK(value.isErrorException(), "expected error raised");
      return *value;
  }

  if (env->next <= env->length) {
    return raiseJSONDecodeError(thread, env, data, env->next - 1, "Extra data");
  }
  return *value;
}

RawObject FUNC(_json, loads)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  DataArray data(&scope, runtime->emptyMutableBytes());
  Object s(&scope, args.get(static_cast<word>(LoadsArg::kString)));
  word length;
  word next = 0;
  if (runtime->isInstanceOfStr(*s)) {
    s = strUnderlying(*s);
    length = Str::cast(*s).length();
  } else if (runtime->isInstanceOfBytes(*s) ||
             runtime->isInstanceOfBytearray(*s)) {
    UNIMPLEMENTED("bytes/bytearray");
  } else {
    return thread->raiseWithFmt(
        LayoutId::kTypeError,
        "the JSON object must be str, bytes or bytearray, not %T", &s);
  }

  if (s.isSmallStr()) {
    DCHECK(length == SmallStr::cast(*s).length(), "length mismatch");
    MutableBytes copy(&scope, runtime->newMutableBytesUninitialized(length));
    copy.replaceFromWithStr(0, Str::cast(*s), length);
    data = *copy;
  } else if (s.isLargeStr()) {
    DCHECK(length == LargeStr::cast(*s).length(), "length mismatch");
    data = LargeStr::cast(*s);
  }

  Dict kw(&scope, args.get(static_cast<word>(LoadsArg::kKw)));
  Object strict_obj(&scope, dictAtById(thread, kw, ID(strict)));
  bool strict;
  bool had_strict = false;
  if (!strict_obj.isErrorNotFound()) {
    if (!runtime->isInstanceOfInt(*strict_obj)) {
      return thread->raiseRequiresType(strict_obj, ID(int));
    }
    had_strict = true;
    strict = !intUnderlying(*strict_obj).isZero();
  } else {
    strict = true;
  }

  Object cls(&scope, args.get(static_cast<word>(LoadsArg::kCls)));
  if (!cls.isNoneType() || kw.numItems() > static_cast<word>(had_strict)) {
    UNIMPLEMENTED("custom cls");
  }

  JSONParser env;
  memset(&env, 0, sizeof(env));
  env.next = next;
  env.length = length;
  env.args = args;
  env.strict = strict;

  if (!args.get(static_cast<word>(LoadsArg::kObjectHook)).isNoneType()) {
    UNIMPLEMENTED("object hook");
  }
  if (!args.get(static_cast<word>(LoadsArg::kParseFloat)).isNoneType()) {
    UNIMPLEMENTED("parse float hook");
  }
  if (!args.get(static_cast<word>(LoadsArg::kParseInt)).isNoneType()) {
    env.has_parse_int = true;
  }
  if (!args.get(static_cast<word>(LoadsArg::kParseConstant)).isNoneType()) {
    env.has_parse_constant = true;
  }
  if (!args.get(static_cast<word>(LoadsArg::kObjectPairsHook)).isNoneType()) {
    UNIMPLEMENTED("object pairs hook");
  }
  return parse(thread, &env, data);
}

}  // namespace py

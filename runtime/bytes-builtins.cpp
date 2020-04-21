#include "bytes-builtins.h"

#include "builtins.h"
#include "bytearray-builtins.h"
#include "frame.h"
#include "int-builtins.h"
#include "runtime.h"
#include "slice-builtins.h"
#include "unicode.h"
#include "utils.h"

namespace py {

RawObject bytesDecodeASCII(Thread* thread, const Bytes& bytes) {
  HandleScope scope(thread);
  if (!bytes.isASCII()) {
    return Unbound::object();
  }
  if (bytes.isSmallBytes()) {
    return SmallBytes::cast(*bytes).becomeStr();
  }
  word bytes_len = bytes.length();
  MutableBytes buf(&scope,
                   thread->runtime()->newMutableBytesUninitialized(bytes_len));
  buf.replaceFromWith(0, *bytes, bytes_len);
  return buf.becomeStr();
}

word bytesCount(const Bytes& haystack, word haystack_len, const Bytes& needle,
                word needle_len, word start, word end) {
  DCHECK_BOUND(haystack_len, haystack.length());
  DCHECK_BOUND(needle_len, needle.length());
  if (start > haystack_len) {
    return 0;
  }
  Slice::adjustSearchIndices(&start, &end, haystack_len);
  if (needle_len == 0) {
    return haystack_len - start + 1;
  }
  word count = 0;
  word index =
      bytesFind(haystack, haystack_len, needle, needle_len, start, end);
  while (index != -1) {
    count++;
    index = bytesFind(haystack, haystack_len, needle, needle_len,
                      index + needle_len, end);
  }
  return count;
}

word bytesFind(const Bytes& haystack, word haystack_len, const Bytes& needle,
               word needle_len, word start, word end) {
  DCHECK_BOUND(haystack_len, haystack.length());
  DCHECK_BOUND(needle_len, needle.length());
  Slice::adjustSearchIndices(&start, &end, haystack_len);
  for (word i = start; i <= end - needle_len; i++) {
    bool has_match = true;
    for (word j = 0; has_match && j < needle_len; j++) {
      has_match = haystack.byteAt(i + j) == needle.byteAt(j);
    }
    if (has_match) {
      return i;
    }
  }
  return -1;
}

RawObject bytesHex(Thread* thread, const Bytes& bytes, word length) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  ByteArray buffer(&scope, runtime->newByteArray());
  runtime->byteArrayEnsureCapacity(thread, buffer, length * 2);
  for (word i = 0; i < length; i++) {
    writeByteAsHexDigits(thread, buffer, bytes.byteAt(i));
  }
  return runtime->newStrFromByteArray(buffer);
}

word bytesRFind(const Bytes& haystack, word haystack_len, const Bytes& needle,
                word needle_len, word start, word end) {
  DCHECK_BOUND(haystack_len, haystack.length());
  DCHECK_BOUND(needle_len, needle.length());
  Slice::adjustSearchIndices(&start, &end, haystack_len);
  for (word i = end - needle_len; i >= start; i--) {
    bool has_match = true;
    for (word j = 0; has_match && j < needle_len; j++) {
      has_match = haystack.byteAt(i + j) == needle.byteAt(j);
    }
    if (has_match) {
      return i;
    }
  }
  return -1;
}

static RawObject bytesReprWithDelimiter(Thread* thread, const Bytes& bytes,
                                        byte delimiter) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  ByteArray buffer(&scope, runtime->newByteArray());
  word len = bytes.length();
  // Each byte will be mapped to one or more ASCII characters. Add 3 to the
  // length for the 2-character prefix (b') and the 1-character suffix (').
  // We expect mostly ASCII bytes, so we usually will not have to resize again.
  runtime->byteArrayEnsureCapacity(thread, buffer, len + 3);
  const byte bytes_delim[] = {'b', delimiter};
  runtime->byteArrayExtend(thread, buffer, bytes_delim);
  for (word i = 0; i < len; i++) {
    byte current = bytes.byteAt(i);
    if (current == delimiter || current == '\\') {
      const byte escaped[] = {'\\', current};
      runtime->byteArrayExtend(thread, buffer, escaped);
    } else if (current == '\t') {
      const byte escaped[] = {'\\', 't'};
      runtime->byteArrayExtend(thread, buffer, escaped);
    } else if (current == '\n') {
      const byte escaped[] = {'\\', 'n'};
      runtime->byteArrayExtend(thread, buffer, escaped);
    } else if (current == '\r') {
      const byte escaped[] = {'\\', 'r'};
      runtime->byteArrayExtend(thread, buffer, escaped);
    } else if (current < ' ' || current >= 0x7f) {
      const byte escaped[] = {'\\', 'x'};
      runtime->byteArrayExtend(thread, buffer, escaped);
      writeByteAsHexDigits(thread, buffer, current);
    } else {
      byteArrayAdd(thread, runtime, buffer, current);
    }
  }
  byteArrayAdd(thread, runtime, buffer, delimiter);
  return runtime->newStrFromByteArray(buffer);
}

RawObject bytesReprSingleQuotes(Thread* thread, const Bytes& bytes) {
  return bytesReprWithDelimiter(thread, bytes, '\'');
}

RawObject bytesReprSmartQuotes(Thread* thread, const Bytes& bytes) {
  word len = bytes.length();
  bool has_single_quote = false;
  for (word i = 0; i < len; i++) {
    switch (bytes.byteAt(i)) {
      case '\'':
        has_single_quote = true;
        break;
      case '"':
        return bytesReprWithDelimiter(thread, bytes, '\'');
      default:
        break;
    }
  }
  return bytesReprWithDelimiter(thread, bytes, has_single_quote ? '"' : '\'');
}

// Returns the index of the first byte in bytes that is not in chars.
static word bytesSpanLeft(const Bytes& bytes, word bytes_len,
                          const Bytes& chars, word chars_len) {
  for (word left = 0; left < bytes_len; left++) {
    byte ch = bytes.byteAt(left);
    bool found_in_chars = false;
    for (word i = 0; i < chars_len; i++) {
      if (ch == chars.byteAt(i)) {
        found_in_chars = true;
        break;
      }
    }
    if (!found_in_chars) {
      return left;
    }
  }
  return bytes_len;
}

// Returns the index of the last byte in bytes that is not in chars. Stops at
// and returns the left bound if all characters to the right were found.
static word bytesSpanRight(const Bytes& bytes, word bytes_len,
                           const Bytes& chars, word chars_len, word left) {
  for (word right = bytes_len; left < right; right--) {
    byte ch = bytes.byteAt(right - 1);
    bool found_in_chars = false;
    for (word i = 0; i < chars_len; i++) {
      if (ch == chars.byteAt(i)) {
        found_in_chars = true;
        break;
      }
    }
    if (!found_in_chars) {
      return right;
    }
  }
  return left;
}

RawObject bytesStrip(Thread* thread, const Bytes& bytes, word bytes_len,
                     const Bytes& chars, word chars_len) {
  word left = bytesSpanLeft(bytes, bytes_len, chars, chars_len);
  word right = bytesSpanRight(bytes, bytes_len, chars, chars_len, left);
  return bytesSubseq(thread, bytes, left, right - left);
}

RawObject bytesStripLeft(Thread* thread, const Bytes& bytes, word bytes_len,
                         const Bytes& chars, word chars_len) {
  word left = bytesSpanLeft(bytes, bytes_len, chars, chars_len);
  return bytesSubseq(thread, bytes, left, bytes_len - left);
}

RawObject bytesStripRight(Thread* thread, const Bytes& bytes, word bytes_len,
                          const Bytes& chars, word chars_len) {
  word right = bytesSpanRight(bytes, bytes_len, chars, chars_len, 0);
  return bytesSubseq(thread, bytes, 0, right);
}

RawObject bytesStripSpace(Thread* thread, const Bytes& bytes, word len) {
  word left = 0;
  while (left < len && ASCII::isSpace(bytes.byteAt(left))) {
    left++;
  }
  word right = len;
  while (right > left && ASCII::isSpace(bytes.byteAt(right - 1))) {
    right--;
  }
  return bytesSubseq(thread, bytes, left, right - left);
}

RawObject bytesStripSpaceLeft(Thread* thread, const Bytes& bytes, word len) {
  word left = 0;
  while (left < len && ASCII::isSpace(bytes.byteAt(left))) {
    left++;
  }
  return bytesSubseq(thread, bytes, left, len - left);
}

RawObject bytesStripSpaceRight(Thread* thread, const Bytes& bytes, word len) {
  word right = len;
  while (right > 0 && ASCII::isSpace(bytes.byteAt(right - 1))) {
    right--;
  }
  return bytesSubseq(thread, bytes, 0, right);
}

RawObject bytesSubseq(Thread* thread, const Bytes& bytes, word start,
                      word length) {
  DCHECK_BOUND(start, bytes.length());
  DCHECK_BOUND(length, bytes.length() - start);
  if (length <= SmallBytes::kMaxLength) {
    byte buffer[SmallBytes::kMaxLength];
    for (word i = length - 1; i >= 0; i--) {
      buffer[i] = bytes.byteAt(start + i);
    }
    return SmallBytes::fromBytes({buffer, length});
  }
  HandleScope scope(thread);
  MutableBytes result(&scope,
                      thread->runtime()->newMutableBytesUninitialized(length));
  result.replaceFromWithStartAt(/*dst_start=*/0, *bytes, length, start);
  return result.becomeImmutable();
}

static bool isUTF8Continuation(byte b) {
  return (b & 0xC0) == 0x80;  // Test for 0b10xxxxxx
}

static bool bytesIsValidUTF8Impl(RawBytes bytes, bool allow_surrogates) {
  for (word i = 0, length = bytes.length(); i < length;) {
    byte b0 = bytes.byteAt(i++);
    // ASCII bytes have the topmost bit zero.
    static_assert(kMaxASCII == 0x7F, "unexpected kMaxASCII value");
    if (b0 <= 0x7F) continue;
    // Bytes past this point have the high bit set (0b1xxxxxxx).

    // 0b110xxxxx begins a sequence with one continuation byte.
    // `b0 < 0b11100000` overestimates and we filter in a 2nd comparison.
    if (b0 < 0xE0) {
      // b0 < 0xC0   catches 0b10xxxxxx bytes (invalid continuation bytes).
      // 0xC0 + 0xC1 (0b11000000 + 0b110000001) would result in range(0x7F)
      // which should have been encoded as ASCII.
      if (b0 < 0xC2) {
        return false;
      }
      if (i >= length) {
        return false;
      }
      byte b1 = bytes.byteAt(i++);
      if (!isUTF8Continuation(b1)) {
        return false;
      }
      if (DCHECK_IS_ON()) {
        uword decoded =
            static_cast<uword>(b0 & 0x1F) << 6 | static_cast<uword>(b1 & 0x3F);
        DCHECK(0x80 <= decoded && decoded <= 0x7FF, "unexpected value");
      }

      // 0b1110xxxx starts a sequence with two continuation bytes.
    } else if (b0 < 0xF0) {
      if (i + 1 >= length) {
        return false;
      }
      byte b1 = bytes.byteAt(i++);
      byte b2 = bytes.byteAt(i++);
      if (!isUTF8Continuation(b1) || !isUTF8Continuation(b2)) {
        return false;
      }

      // Catch sequences that should have been encoded in 1-2 bytes instead.
      if (b0 == 0xE0) {
        if (b1 < 0xA0) {
          return false;
        }
      } else if (!allow_surrogates && b0 == 0xED && b1 >= 0xA0) {
        // 0b11011xxxxxxxxxxx  (0xD800 - 0xDFFF) is declared invalid by unicode
        // as they look like utf-16 surrogates making it easier to detect
        // mix-ups.
        return false;
      }

      if (DCHECK_IS_ON()) {
        uword decoded = static_cast<uword>(b0 & 0x0F) << 12 |
                        static_cast<uword>(b1 & 0x3F) << 6 |
                        static_cast<uword>(b2 & 0x3F);
        DCHECK(0x0800 <= decoded && decoded <= 0xFFFF, "unexpected value");
      }

      static_assert(kMaxUnicode == 0x10FFFF, "unexpected maxunicode value");
      // 0b11110xxx starts a sequence with three continuation bytes.
      // However values bigger than 0x10FFFF are not valid unicode, so we test
      // b0 < 0b11110101 to overestimate that.
    } else if (b0 < 0xF5) {
      if (i + 2 >= length) {
        return false;
      }
      byte b1 = bytes.byteAt(i++);
      byte b2 = bytes.byteAt(i++);
      byte b3 = bytes.byteAt(i++);
      if (!isUTF8Continuation(b1) || !isUTF8Continuation(b2) ||
          !isUTF8Continuation(b3)) {
        return false;
      }
      // Catch sequences that should have been encoded with 1-3 bytes instead.
      if (b0 == 0xF0) {
        if (b1 < 0x90) {
          return false;
        }
      } else if (b0 == 0xF4 && b1 >= 0x90) {
        // Bigger than kMaxUnicode.
        return false;
      }

      if (DCHECK_IS_ON()) {
        uword decoded = static_cast<uword>(b0 & 0x07) << 16 |
                        static_cast<uword>(b1 & 0x3F) << 12 |
                        static_cast<uword>(b2 & 0x3F) << 6 |
                        static_cast<uword>(b3 & 0x3F);
        DCHECK(0x10000 <= decoded && decoded <= kMaxUnicode,
               "unexpected value");
      }
    } else {
      // Invalid prefix byte.
      return false;
    }
  }
  return true;
}

bool bytesIsValidUTF8(RawBytes bytes) {
  return bytesIsValidUTF8Impl(bytes, /*allow_surrogates=*/false);
}

bool bytesIsValidStr(RawBytes bytes) {
  return bytesIsValidUTF8Impl(bytes, /*allow_surrogates=*/true);
}

void SmallBytesBuiltins::postInitialize(Runtime* runtime,
                                        const Type& new_type) {
  runtime->setSmallBytesType(new_type);
  Layout::cast(new_type.instanceLayout())
      .setDescribedType(runtime->typeAt(kSuperType));
}

void LargeBytesBuiltins::postInitialize(Runtime* runtime,
                                        const Type& new_type) {
  new_type.setBuiltinBase(kSuperType);
  runtime->setLargeBytesType(new_type);
  Layout::cast(new_type.instanceLayout())
      .setDescribedType(runtime->typeAt(kSuperType));
}

// Used only for UserBytesBase as a heap-allocated object.
const BuiltinAttribute BytesBuiltins::kAttributes[] = {
    {ID(_UserBytes__value), UserBytesBase::kValueOffset,
     AttributeFlags::kHidden},
    {SymbolId::kSentinelId, 0},
};

void BytesBuiltins::postInitialize(Runtime*, const Type& new_type) {
  new_type.setBuiltinBase(LayoutId::kBytes);
}

RawObject METH(bytes, __add__)(Thread* thread, Frame* frame, word nargs) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfBytes(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(bytes));
  }
  Bytes self(&scope, bytesUnderlying(*self_obj));
  Object other_obj(&scope, args.get(1));
  if (runtime->isInstanceOfBytes(*other_obj)) {
    Bytes other(&scope, bytesUnderlying(*other_obj));
    return runtime->bytesConcat(thread, self, other);
  }
  if (runtime->isInstanceOfByteArray(*other_obj)) {
    ByteArray other(&scope, *other_obj);
    Bytes other_bytes(&scope, byteArrayAsBytes(thread, other));
    return runtime->bytesConcat(thread, self, other_bytes);
  }
  // TODO(T38246066): buffers besides bytes/bytearray
  return thread->raiseWithFmt(LayoutId::kTypeError, "can't concat %T to bytes",
                              &other_obj);
}

RawObject METH(bytes, __eq__)(Thread* thread, Frame* frame, word nargs) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfBytes(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(bytes));
  }
  Object other_obj(&scope, args.get(1));
  if (!runtime->isInstanceOfBytes(*other_obj)) {
    return NotImplementedType::object();
  }
  Bytes self(&scope, bytesUnderlying(*self_obj));
  Bytes other(&scope, bytesUnderlying(*other_obj));
  return Bool::fromBool(self.compare(*other) == 0);
}

RawObject METH(bytes, __ge__)(Thread* thread, Frame* frame, word nargs) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfBytes(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(bytes));
  }
  Object other_obj(&scope, args.get(1));
  if (!runtime->isInstanceOfBytes(*other_obj)) {
    return NotImplementedType::object();
  }
  Bytes self(&scope, bytesUnderlying(*self_obj));
  Bytes other(&scope, bytesUnderlying(*other_obj));
  return Bool::fromBool(self.compare(*other) >= 0);
}

RawObject METH(bytes, __gt__)(Thread* thread, Frame* frame, word nargs) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfBytes(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(bytes));
  }
  Object other_obj(&scope, args.get(1));
  if (!runtime->isInstanceOfBytes(*other_obj)) {
    return NotImplementedType::object();
  }
  Bytes self(&scope, bytesUnderlying(*self_obj));
  Bytes other(&scope, bytesUnderlying(*other_obj));
  return Bool::fromBool(self.compare(*other) > 0);
}

RawObject METH(bytes, __hash__)(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfBytes(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(bytes));
  }
  Bytes self(&scope, bytesUnderlying(*self_obj));
  return SmallInt::fromWord(bytesHash(thread, *self));
}

RawObject METH(bytes, __iter__)(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfBytes(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(bytes));
  }
  Bytes self(&scope, bytesUnderlying(*self_obj));
  return runtime->newBytesIterator(thread, self);
}

RawObject METH(bytes, __le__)(Thread* thread, Frame* frame, word nargs) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfBytes(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(bytes));
  }
  Object other_obj(&scope, args.get(1));
  if (!runtime->isInstanceOfBytes(*other_obj)) {
    return NotImplementedType::object();
  }
  Bytes self(&scope, bytesUnderlying(*self_obj));
  Bytes other(&scope, bytesUnderlying(*other_obj));
  return Bool::fromBool(self.compare(*other) <= 0);
}

RawObject METH(bytes, __len__)(Thread* thread, Frame* frame, word nargs) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfBytes(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(bytes));
  }

  Bytes self(&scope, bytesUnderlying(*self_obj));
  return SmallInt::fromWord(self.length());
}

RawObject METH(bytes, __lt__)(Thread* thread, Frame* frame, word nargs) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfBytes(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(bytes));
  }
  Object other_obj(&scope, args.get(1));
  if (!runtime->isInstanceOfBytes(*other_obj)) {
    return NotImplementedType::object();
  }
  Bytes self(&scope, bytesUnderlying(*self_obj));
  Bytes other(&scope, bytesUnderlying(*other_obj));
  return Bool::fromBool(self.compare(*other) < 0);
}

RawObject METH(bytes, __mul__)(Thread* thread, Frame* frame, word nargs) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfBytes(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(bytes));
  }
  Object count_index(&scope, args.get(1));
  Object count_obj(&scope, intFromIndex(thread, count_index));
  if (count_obj.isError()) return *count_obj;
  Bytes self(&scope, bytesUnderlying(*self_obj));
  word count = intUnderlying(*count_obj).asWordSaturated();
  if (!SmallInt::isValid(count)) {
    return thread->raiseWithFmt(LayoutId::kOverflowError,
                                "cannot fit '%T' into an index-sized integer",
                                &count_obj);
  }
  word length = self.length();
  if (count <= 0 || length == 0) {
    return Bytes::empty();
  }
  if (count == 1) {
    return *self;
  }
  word new_length;
  if (__builtin_mul_overflow(length, count, &new_length) ||
      !SmallInt::isValid(new_length)) {
    return thread->raiseWithFmt(LayoutId::kOverflowError,
                                "repeated bytes are too long");
  }
  return runtime->bytesRepeat(thread, self, length, count);
}

RawObject METH(bytes, __ne__)(Thread* thread, Frame* frame, word nargs) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfBytes(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(bytes));
  }
  Object other_obj(&scope, args.get(1));
  if (!runtime->isInstanceOfBytes(*other_obj)) {
    return NotImplementedType::object();
  }
  Bytes self(&scope, bytesUnderlying(*self_obj));
  Bytes other(&scope, bytesUnderlying(*other_obj));
  return Bool::fromBool(self.compare(*other) != 0);
}

RawObject METH(bytes, __repr__)(Thread* thread, Frame* frame, word nargs) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfBytes(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(bytes));
  }
  Bytes self(&scope, bytesUnderlying(*self_obj));
  return bytesReprSmartQuotes(thread, self);
}

RawObject METH(bytes, hex)(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object obj(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfBytes(*obj)) {
    return thread->raiseRequiresType(obj, ID(bytes));
  }
  Bytes self(&scope, bytesUnderlying(*obj));
  return bytesHex(thread, self, self.length());
}

RawObject METH(bytes, lower)(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfBytes(*self)) {
    return thread->raiseRequiresType(self, ID(bytes));
  }
  self = bytesUnderlying(*self);
  if (self.isSmallBytes()) {
    SmallBytes small_bytes(&scope, *self);
    word length = small_bytes.length();
    byte buffer[SmallBytes::kMaxLength];
    small_bytes.copyTo(buffer, length);
    for (word i = 0; i < length; i++) {
      buffer[i] = ASCII::toLower(buffer[i]);
    }
    return SmallBytes::fromBytes(View<byte>(buffer, length));
  }
  LargeBytes large_bytes(&scope, *self);
  word length = large_bytes.length();
  MutableBytes result(&scope, runtime->newMutableBytesUninitialized(length));
  for (word i = 0; i < length; i++) {
    result.byteAtPut(i, ASCII::toLower(large_bytes.byteAt(i)));
  }
  return result.becomeImmutable();
}

RawObject METH(bytes, lstrip)(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfBytes(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(bytes));
  }
  Bytes self(&scope, bytesUnderlying(*self_obj));
  Object chars_obj(&scope, args.get(1));
  if (chars_obj.isNoneType()) {
    return bytesStripSpaceLeft(thread, self, self.length());
  }
  if (runtime->isInstanceOfBytes(*chars_obj)) {
    Bytes chars(&scope, bytesUnderlying(*chars_obj));
    return bytesStripLeft(thread, self, self.length(), chars, chars.length());
  }
  if (runtime->isInstanceOfByteArray(*chars_obj)) {
    ByteArray chars(&scope, *chars_obj);
    Bytes chars_bytes(&scope, chars.items());
    return bytesStripLeft(thread, self, self.length(), chars_bytes,
                          chars.numItems());
  }
  // TODO(T38246066): support bytes-like objects other than bytes, bytearray
  return thread->raiseWithFmt(LayoutId::kTypeError,
                              "a bytes-like object is required, not '%T'",
                              &chars_obj);
}

RawObject METH(bytes, rstrip)(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfBytes(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(bytes));
  }
  Bytes self(&scope, bytesUnderlying(*self_obj));
  Object chars_obj(&scope, args.get(1));
  if (chars_obj.isNoneType()) {
    return bytesStripSpaceRight(thread, self, self.length());
  }
  if (runtime->isInstanceOfBytes(*chars_obj)) {
    Bytes chars(&scope, bytesUnderlying(*chars_obj));
    return bytesStripRight(thread, self, self.length(), chars, chars.length());
  }
  if (runtime->isInstanceOfByteArray(*chars_obj)) {
    ByteArray chars(&scope, *chars_obj);
    Bytes chars_bytes(&scope, chars.items());
    return bytesStripRight(thread, self, self.length(), chars_bytes,
                           chars.numItems());
  }
  // TODO(T38246066): support bytes-like objects other than bytes, bytearray
  return thread->raiseWithFmt(LayoutId::kTypeError,
                              "a bytes-like object is required, not '%T'",
                              &chars_obj);
}

RawObject METH(bytes, strip)(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfBytes(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(bytes));
  }
  Bytes self(&scope, bytesUnderlying(*self_obj));
  Object chars_obj(&scope, args.get(1));
  if (chars_obj.isNoneType()) {
    return bytesStripSpace(thread, self, self.length());
  }
  if (runtime->isInstanceOfBytes(*chars_obj)) {
    Bytes chars(&scope, bytesUnderlying(*chars_obj));
    return bytesStrip(thread, self, self.length(), chars, chars.length());
  }
  if (runtime->isInstanceOfByteArray(*chars_obj)) {
    ByteArray chars(&scope, *chars_obj);
    Bytes chars_bytes(&scope, chars.items());
    return bytesStrip(thread, self, self.length(), chars_bytes,
                      chars.numItems());
  }
  // TODO(T38246066): support bytes-like objects other than bytes, bytearray
  return thread->raiseWithFmt(LayoutId::kTypeError,
                              "a bytes-like object is required, not '%T'",
                              &chars_obj);
}

RawObject METH(bytes, translate)(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfBytes(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(bytes));
  }
  Bytes self(&scope, bytesUnderlying(*self_obj));
  Object table_obj(&scope, args.get(1));
  word table_length;
  if (table_obj.isNoneType()) {
    table_length = BytesBuiltins::kTranslationTableLength;
    table_obj = Bytes::empty();
  } else if (runtime->isInstanceOfBytes(*table_obj)) {
    Bytes bytes(&scope, bytesUnderlying(*table_obj));
    table_length = bytes.length();
    table_obj = *bytes;
  } else if (runtime->isInstanceOfByteArray(*table_obj)) {
    ByteArray array(&scope, *table_obj);
    table_length = array.numItems();
    table_obj = array.items();
  } else {
    // TODO(T38246066): allow any bytes-like object
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "a bytes-like object is required, not '%T'",
                                &table_obj);
  }
  if (table_length != BytesBuiltins::kTranslationTableLength) {
    return thread->raiseWithFmt(LayoutId::kValueError,
                                "translation table must be %w characters long",
                                BytesBuiltins::kTranslationTableLength);
  }
  Bytes table(&scope, *table_obj);
  Object del(&scope, args.get(2));
  if (runtime->isInstanceOfBytes(*del)) {
    Bytes bytes(&scope, bytesUnderlying(*del));
    return runtime->bytesTranslate(thread, self, self.length(), table,
                                   table_length, bytes, bytes.length());
  }
  if (runtime->isInstanceOfByteArray(*del)) {
    ByteArray array(&scope, *del);
    Bytes bytes(&scope, array.items());
    return runtime->bytesTranslate(thread, self, self.length(), table,
                                   table_length, bytes, array.numItems());
  }
  // TODO(T38246066): allow any bytes-like object
  return thread->raiseWithFmt(
      LayoutId::kTypeError, "a bytes-like object is required, not '%T'", &del);
}

RawObject METH(bytes, upper)(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfBytes(*self)) {
    return thread->raiseRequiresType(self, ID(bytes));
  }
  self = bytesUnderlying(*self);
  if (self.isSmallBytes()) {
    SmallBytes small_bytes(&scope, *self);
    word length = small_bytes.length();
    byte buffer[SmallBytes::kMaxLength];
    small_bytes.copyTo(buffer, length);
    for (word i = 0; i < length; i++) {
      buffer[i] = ASCII::toUpper(buffer[i]);
    }
    return SmallBytes::fromBytes(View<byte>(buffer, length));
  }
  LargeBytes large_bytes(&scope, *self);
  word length = large_bytes.length();
  MutableBytes result(&scope, runtime->newMutableBytesUninitialized(length));
  for (word i = 0; i < length; i++) {
    result.byteAtPut(i, ASCII::toUpper(large_bytes.byteAt(i)));
  }
  return result.becomeImmutable();
}

RawObject METH(bytes_iterator, __iter__)(Thread* thread, Frame* frame,
                                         word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self.isBytesIterator()) {
    return thread->raiseRequiresType(self, ID(bytes_iterator));
  }
  return *self;
}

RawObject METH(bytes_iterator, __next__)(Thread* thread, Frame* frame,
                                         word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self.isBytesIterator()) {
    return thread->raiseRequiresType(self, ID(bytes_iterator));
  }
  BytesIterator iter(&scope, *self);
  Bytes underlying(&scope, iter.iterable());
  word index = iter.index();
  if (index >= underlying.length()) {
    return thread->raise(LayoutId::kStopIteration, NoneType::object());
  }
  iter.setIndex(index + 1);
  return SmallInt::fromWord(underlying.byteAt(index));
}

RawObject METH(bytes_iterator, __length_hint__)(Thread* thread, Frame* frame,
                                                word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self.isBytesIterator()) {
    return thread->raiseRequiresType(self, ID(bytes_iterator));
  }
  BytesIterator iter(&scope, *self);
  Bytes underlying(&scope, iter.iterable());
  return SmallInt::fromWord(underlying.length() - iter.index());
}

}  // namespace py

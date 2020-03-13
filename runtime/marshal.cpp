#include "marshal.h"

#include <cstdlib>
#include <cstring>
#include <memory>

#include "heap.h"
#include "runtime.h"
#include "set-builtins.h"
#include "tuple-builtins.h"
#include "utils.h"
#include "view.h"

namespace py {

const int32_t kPycMagic = 3379 | (int32_t{'\r'} << 16) | (int32_t{'\n'} << 24);

enum {
  FLAG_REF = '\x80',  // with a type, add obj to index
  TYPE_ASCII = 'a',
  TYPE_ASCII_INTERNED = 'A',
  TYPE_BINARY_COMPLEX = 'y',
  TYPE_BINARY_FLOAT = 'g',
  TYPE_CODE = 'c',
  TYPE_COMPLEX = 'x',
  TYPE_DICT = '{',
  TYPE_ELLIPSIS = '.',
  TYPE_FALSE = 'F',
  TYPE_FLOAT = 'f',
  TYPE_FROZENSET = '>',
  TYPE_INTERNED = 't',
  TYPE_INT = 'i',
  TYPE_LIST = '[',
  TYPE_LONG = 'l',
  TYPE_NONE = 'N',
  TYPE_NULL = '0',
  TYPE_REF = 'r',
  TYPE_SET = '<',
  TYPE_SHORT_ASCII_INTERNED = 'Z',
  TYPE_SHORT_ASCII = 'z',
  TYPE_SMALL_TUPLE = ')',
  TYPE_STOPITER = 'S',
  TYPE_STRING = 's',
  TYPE_TRUE = 'T',
  TYPE_TUPLE = '(',
  TYPE_UNICODE = 'u',
  TYPE_UNKNOWN = '?',
};

Marshal::Reader::Reader(HandleScope* scope, Runtime* runtime, View<byte> buffer)
    : runtime_(runtime),
      refs_(scope, runtime->newList()),
      start_(buffer.data()),
      depth_(0),
      length_(buffer.length()),
      pos_(0) {
  end_ = start_ + length_;
}

RawObject Marshal::Reader::readPycHeader(const Str& filename) {
  Thread* thread = Thread::current();
  const word header_length = 12;
  if (length_ - pos_ < header_length) {
    return thread->raiseWithFmt(
        LayoutId::kEOFError, "reached end of file while reading header of '%S'",
        &filename);
  }
  int32_t magic = readLong();
  if (magic != kPycMagic) {
    return thread->raiseWithFmt(LayoutId::kImportError,
                                "unsupported magic number in '%S'", &filename);
  }
  readLong();  // read source timestamp.
  readLong();  // read source length.
  DCHECK(pos_ == header_length, "size mismatch");
  return NoneType::object();
}

void Marshal::Reader::setBuiltinFunctions(
    const Function::Entry* builtin_functions, word num_builtin_functions) {
  builtin_functions_ = builtin_functions;
  num_builtin_functions_ = num_builtin_functions;
}

const byte* Marshal::Reader::readBytes(int length) {
  const byte* result = &start_[pos_];
  pos_ += length;
  return result;
}

byte Marshal::Reader::readByte() {
  byte result = 0xFF;
  const byte* buffer = readBytes(1);
  if (buffer != nullptr) {
    result = buffer[0];
  }
  return result;
}

int16_t Marshal::Reader::readShort() {
  int16_t result = -1;
  const byte* buffer = readBytes(sizeof(result));
  if (buffer != nullptr) {
    result = buffer[0];
    result |= buffer[1] << 8;
  }
  return result;
}

int32_t Marshal::Reader::readLong() {
  int32_t result = -1;
  const byte* buffer = readBytes(4);
  if (buffer != nullptr) {
    result = buffer[0];
    result |= buffer[1] << 8;
    result |= buffer[2] << 16;
    result |= buffer[3] << 24;
  }
  return result;
}

double Marshal::Reader::readBinaryFloat() {
  double result;
  const byte* buffer = readBytes(sizeof(result));
  std::memcpy(&result, buffer, sizeof(result));
  return result;
}

template <typename T>
class ScopedCounter {
 public:
  explicit ScopedCounter(T* counter) : counter_(counter) { (*counter_)++; }
  ~ScopedCounter() { (*counter_)--; }

 public:
  T* counter_;
  DISALLOW_COPY_AND_ASSIGN(ScopedCounter);
};

RawObject Marshal::Reader::readObject() {
  ScopedCounter<int> counter(&depth_);
  byte code = readByte();
  byte flag = code & FLAG_REF;
  byte type = code & ~FLAG_REF;
  isRef_ = flag;
  switch (type) {
    case TYPE_NULL:
      return SmallInt::fromWord(0);

    case TYPE_NONE:
      return NoneType::object();

    case TYPE_STOPITER:
      UNIMPLEMENTED("TYPE_STOPITER");

    case TYPE_ELLIPSIS:
      return runtime_->ellipsis();

    case TYPE_FALSE:
      return Bool::falseObj();

    case TYPE_TRUE:
      return Bool::trueObj();

    case TYPE_INT: {
      // NB: this will continue to work as long as SmallInt can contain the
      // full range of 32 bit signed integer values. Notably, this will break if
      // we need to support 32 bit machines.
      word n = readLong();
      if (!SmallInt::isValid(n)) {
        UNIMPLEMENTED("value '%ld' outside range supported by RawSmallInt", n);
      }
      RawObject result = SmallInt::fromWord(n);
      if (isRef_) {
        addRef(result);
      }
      return result;
    }

    case TYPE_FLOAT:
      UNIMPLEMENTED("TYPE_FLOAT");

    case TYPE_BINARY_FLOAT: {
      double n = readBinaryFloat();
      RawObject result = runtime_->newFloat(n);
      if (isRef_) {
        addRef(result);
      }
      return result;
    }

    case TYPE_COMPLEX:
      UNIMPLEMENTED("TYPE_COMPLEX");

    case TYPE_BINARY_COMPLEX: {
      double real = readBinaryFloat();
      double imag = readBinaryFloat();
      RawObject result = runtime_->newComplex(real, imag);
      if (isRef_) {
        addRef(result);
      }
      return result;
    }

    case TYPE_STRING:  // Misnomer, should be TYPE_BYTES
      return readTypeString();

    case TYPE_INTERNED:
    case TYPE_ASCII_INTERNED:
      return readTypeAsciiInterned();

    case TYPE_UNICODE:
    case TYPE_ASCII: {
      return readTypeAscii();
    }

    case TYPE_SHORT_ASCII_INTERNED:
      return readTypeShortAsciiInterned();

    case TYPE_SHORT_ASCII:
      return readTypeShortAscii();

    case TYPE_SMALL_TUPLE:
      return readTypeSmallTuple();

    case TYPE_TUPLE:
      return readTypeTuple();

    case TYPE_LIST:
      UNIMPLEMENTED("TYPE_LIST");

    case TYPE_DICT:
      UNIMPLEMENTED("TYPE_DICT");

    case TYPE_SET:
      return readTypeSet();

    case TYPE_FROZENSET:
      return readTypeFrozenSet();

    case TYPE_CODE:
      return readTypeCode();

    case TYPE_REF:
      return readTypeRef();

    case TYPE_LONG:
      return readLongObject();

    default:
      UNREACHABLE("unknown type '%c' (flags=%x)", type, flag);
  }
  UNREACHABLE("all cases should be covered");
}

word Marshal::Reader::addRef(RawObject value) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object value_handle(&scope, value);
  word result = refs_.numItems();
  runtime_->listAdd(thread, refs_, value_handle);
  return result;
}

void Marshal::Reader::setRef(word index, RawObject value) {
  refs_.atPut(index, value);
}

RawObject Marshal::Reader::getRef(word index) { return refs_.at(index); }

word Marshal::Reader::numRefs() { return refs_.numItems(); }

RawObject Marshal::Reader::readTypeString() {
  int32_t length = readLong();
  const byte* data = readBytes(length);
  RawObject result = runtime_->newBytesWithAll(View<byte>(data, length));
  if (isRef_) {
    addRef(result);
  }
  return result;
}

RawObject Marshal::Reader::readTypeAscii() {
  word length = readLong();
  if (length < 0) {
    return Thread::current()->raiseWithFmt(
        LayoutId::kValueError, "bad marshal data (string size out of range)");
  }
  return readStr(length);
}

RawObject Marshal::Reader::readTypeAsciiInterned() {
  word length = readLong();
  if (length < 0) {
    return Thread::current()->raiseWithFmt(
        LayoutId::kValueError, "bad marshal data (string size out of range)");
  }
  return readAndInternStr(length);
}

RawObject Marshal::Reader::readTypeShortAscii() {
  word length = readByte();
  return readStr(length);
}

RawObject Marshal::Reader::readTypeShortAsciiInterned() {
  word length = readByte();
  return readAndInternStr(length);
}

RawObject Marshal::Reader::readStr(word length) {
  const byte* data = readBytes(length);
  RawObject result = runtime_->newStrWithAll(View<byte>(data, length));
  if (isRef_) {
    addRef(result);
  }
  return result;
}

RawObject Marshal::Reader::readAndInternStr(word length) {
  const byte* data = readBytes(length);
  Thread* thread = Thread::current();
  RawObject result =
      Runtime::internStrFromAll(thread, View<byte>(data, length));
  if (isRef_) {
    addRef(result);
  }
  return result;
}

RawObject Marshal::Reader::readTypeSmallTuple() {
  int32_t n = readByte();
  return doTupleElements(n);
}

RawObject Marshal::Reader::readTypeTuple() {
  int32_t n = readLong();
  return doTupleElements(n);
}

RawObject Marshal::Reader::doTupleElements(int32_t length) {
  RawObject result = runtime_->newTuple(length);
  if (isRef_) {
    addRef(result);
  }
  for (int32_t i = 0; i < length; i++) {
    RawObject value = readObject();
    Tuple::cast(result).atPut(i, value);
  }
  return result;
}

RawObject Marshal::Reader::readTypeSet() {
  int32_t n = readLong();
  return doSetElements(n, runtime_->newSet());
}

RawObject Marshal::Reader::readTypeFrozenSet() {
  int32_t n = readLong();
  if (n == 0) {
    return runtime_->emptyFrozenSet();
  }
  return doSetElements(n, runtime_->newFrozenSet());
}

RawObject Marshal::Reader::doSetElements(int32_t length, RawObject set_obj) {
  if (isRef_) {
    addRef(set_obj);
  }
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  SetBase set(&scope, set_obj);
  Object value(&scope, NoneType::object());
  Object hash_obj(&scope, NoneType::object());
  for (int32_t i = 0; i < length; i++) {
    value = readObject();
    hash_obj = Interpreter::hash(thread, value);
    DCHECK(!hash_obj.isErrorException(), "must be hashable");
    word hash = SmallInt::cast(*hash_obj).value();
    RawObject result = setAdd(thread, set, value, hash);
    if (result.isError()) {
      return result;
    }
  }
  return *set;
}

RawObject Marshal::Reader::readTypeCode() {
  word index = -1;
  if (isRef_) {
    index = addRef(NoneType::object());
  }
  HandleScope scope;
  int32_t argcount = readLong();
  int32_t posonlyargcount = 0;
  int32_t kwonlyargcount = readLong();
  int32_t nlocals = readLong();
  int32_t stacksize = readLong();
  int32_t flags = readLong();
  CHECK(flags <= (Code::Flags::kLast << 1) - 1, "unknown flags in code object");
  Object code(&scope, readObject());
  Object consts(&scope, readObject());
  Object names(&scope, readObject());
  Tuple varnames(&scope, readObject());
  Tuple freevars(&scope, readObject());
  Tuple cellvars(&scope, readObject());
  Object filename(&scope, readObject());
  Object name(&scope, readObject());
  int32_t firstlineno = readLong();
  Object lnotab(&scope, readObject());

  Object result(&scope, NoneType::object());
  if (flags & Code::Flags::kBuiltin) {
    word function_index = stacksize;
    CHECK(code.isBytes() && Bytes::cast(*code).length() == 0,
          "must not have bytecode in native code");
    CHECK(consts.isTuple() && Tuple::cast(*consts).length() == 0,
          "must not have constants in native code");
    CHECK(names.isTuple() && Tuple::cast(*names).length() == 0,
          "must not have variables in native code");
    CHECK(freevars.length() == 0, "must not have free vars in native code");
    CHECK(cellvars.length() == 0, "must not have cell vars in native code");
    CHECK_INDEX(function_index, num_builtin_functions_);
    Function::Entry entry = builtin_functions_[function_index];
    result = runtime_->newBuiltinCode(argcount, posonlyargcount, kwonlyargcount,
                                      flags, entry, varnames, name);
    Code::cast(*result).setFilename(*filename);
    Code::cast(*result).setFirstlineno(firstlineno);
  } else {
    result = runtime_->newCode(argcount, posonlyargcount, kwonlyargcount,
                               nlocals, stacksize, flags, code, consts, names,
                               varnames, freevars, cellvars, filename, name,
                               firstlineno, lnotab);
  }
  if (index >= 0) {
    setRef(index, *result);
  }
  return *result;
}

RawObject Marshal::Reader::readTypeRef() {
  int32_t n = readLong();
  return getRef(n);
}

RawObject Marshal::Reader::readLongObject() {
  int32_t n = readLong();
  if (n == 0) {
    RawObject zero = SmallInt::fromWord(0);
    if (isRef_) {
      addRef(zero);
    }
    return zero;
  }
  if (n < kMinInt32 || n > kMaxInt32) {
    return Thread::current()->raiseWithFmt(
        LayoutId::kValueError, "bad marshal data (string size out of range)");
  }
  word bits_consumed = 0;
  word n_bits = std::abs(n) * kBitsPerLongDigit;
  word num_digits = ((n_bits + kBitsPerWord + 1) / kBitsPerWord) + 1;
  std::unique_ptr<uword[]> digits{new uword[num_digits]};
  word digits_idx = 0;
  uword buf = 0;
  word word_offset = 0;
  while (bits_consumed < n_bits) {
    int16_t digit = readShort();
    if (digit < 0) {
      return Thread::current()->raiseWithFmt(
          LayoutId::kValueError, "bad marshal data (negative long digit)");
    }
    auto unsigned_digit = static_cast<uword>(digit);
    if (word_offset + kBitsPerLongDigit <= kBitsPerWord) {
      buf |= unsigned_digit << word_offset;
      word_offset += kBitsPerLongDigit;
      if (word_offset == kBitsPerWord) {
        digits[digits_idx++] = buf;
        buf = 0;
        word_offset = 0;
      }
    } else {
      word extra_bits = (word_offset + kBitsPerLongDigit) % kBitsPerWord;
      word bits_to_include = kBitsPerLongDigit - extra_bits;
      buf |= (unsigned_digit & ((1 << bits_to_include) - 1)) << word_offset;
      digits[digits_idx++] = buf;
      buf = (unsigned_digit >> bits_to_include) & ((1 << extra_bits) - 1);
      word_offset = extra_bits;
    }
    bits_consumed += kBitsPerLongDigit;
  }
  if (word_offset > 0 && buf != 0) {
    digits[digits_idx++] = buf;
  } else if (n > 0 && (digits[digits_idx - 1] >> (kBitsPerWord - 1))) {
    // Zero extend if the MSB is set in the top digit and either the result is
    // positive or the top digit has at least one other bit set (in which case
    // we need the extra digit for the negation).
    digits[digits_idx++] = 0;
  }
  if (n < 0) {
    uword carry = 1;
    for (word i = 0; i < digits_idx; i++) {
      uword digit = digits[i];
      carry = __builtin_uaddl_overflow(~digit, carry, &digit);
      digits[i] = digit;
    }
    DCHECK(carry == 0, "Carry should be zero");
    if ((digits[digits_idx - 1] >> (kBitsPerWord - 1)) == 0) {
      digits[digits_idx++] = kMaxUword;
    }
  }

  RawObject result =
      runtime_->newIntWithDigits(View<uword>(digits.get(), digits_idx));
  if (isRef_) {
    addRef(result);
  }
  return result;
}

}  // namespace py

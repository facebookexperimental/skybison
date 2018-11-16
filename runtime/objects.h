#pragma once

#include <limits>

#include "globals.h"
#include "utils.h"
#include "view.h"

namespace python {

#define INTRINSIC_IMMEDIATE_CLASS_NAMES(V)                                     \
  V(SmallInt)                                                                  \
  V(SmallStr)                                                                  \
  V(Bool)                                                                      \
  V(NoneType)

#define INTRINSIC_HEAP_CLASS_NAMES(V)                                          \
  V(Object)                                                                    \
  V(ArithmeticError)                                                           \
  V(AssertionError)                                                            \
  V(AttributeError)                                                            \
  V(BaseException)                                                             \
  V(BlockingIOError)                                                           \
  V(BoundMethod)                                                               \
  V(BrokenPipeError)                                                           \
  V(BufferError)                                                               \
  V(Bytes)                                                                     \
  V(BytesWarning)                                                              \
  V(ChildProcessError)                                                         \
  V(ClassMethod)                                                               \
  V(Code)                                                                      \
  V(Complex)                                                                   \
  V(ConnectionAbortedError)                                                    \
  V(ConnectionError)                                                           \
  V(ConnectionRefusedError)                                                    \
  V(ConnectionResetError)                                                      \
  V(Coroutine)                                                                 \
  V(DeprecationWarning)                                                        \
  V(Dict)                                                                      \
  V(EOFError)                                                                  \
  V(Ellipsis)                                                                  \
  V(Exception)                                                                 \
  V(FileExistsError)                                                           \
  V(FileNotFoundError)                                                         \
  V(Float)                                                                     \
  V(FloatingPointError)                                                        \
  V(Function)                                                                  \
  V(FutureWarning)                                                             \
  V(Generator)                                                                 \
  V(GeneratorExit)                                                             \
  V(HeapFrame)                                                                 \
  V(ImportError)                                                               \
  V(ImportWarning)                                                             \
  V(IndentationError)                                                          \
  V(IndexError)                                                                \
  V(Int)                                                                       \
  V(InterruptedError)                                                          \
  V(IsADirectoryError)                                                         \
  V(KeyError)                                                                  \
  V(KeyboardInterrupt)                                                         \
  V(LargeInt)                                                                  \
  V(LargeStr)                                                                  \
  V(Layout)                                                                    \
  V(List)                                                                      \
  V(ListIterator)                                                              \
  V(LookupError)                                                               \
  V(MemoryError)                                                               \
  V(Module)                                                                    \
  V(ModuleNotFoundError)                                                       \
  V(NameError)                                                                 \
  V(NotADirectoryError)                                                        \
  V(NotImplemented)                                                            \
  V(NotImplementedError)                                                       \
  V(OSError)                                                                   \
  V(ObjectArray)                                                               \
  V(OverflowError)                                                             \
  V(PendingDeprecationWarning)                                                 \
  V(PermissionError)                                                           \
  V(ProcessLookupError)                                                        \
  V(Property)                                                                  \
  V(Range)                                                                     \
  V(RangeIterator)                                                             \
  V(RecursionError)                                                            \
  V(ReferenceError)                                                            \
  V(ResourceWarning)                                                           \
  V(RuntimeError)                                                              \
  V(RuntimeWarning)                                                            \
  V(Set)                                                                       \
  V(SetIterator)                                                               \
  V(Slice)                                                                     \
  V(StaticMethod)                                                              \
  V(StopAsyncIteration)                                                        \
  V(StopIteration)                                                             \
  V(Str)                                                                       \
  V(Super)                                                                     \
  V(SyntaxError)                                                               \
  V(SyntaxWarning)                                                             \
  V(SystemError)                                                               \
  V(SystemExit)                                                                \
  V(TabError)                                                                  \
  V(TimeoutError)                                                              \
  V(TupleIterator)                                                             \
  V(Type)                                                                      \
  V(TypeError)                                                                 \
  V(UnboundLocalError)                                                         \
  V(UnicodeDecodeError)                                                        \
  V(UnicodeEncodeError)                                                        \
  V(UnicodeError)                                                              \
  V(UnicodeTranslateError)                                                     \
  V(UnicodeWarning)                                                            \
  V(UserWarning)                                                               \
  V(ValueCell)                                                                 \
  V(ValueError)                                                                \
  V(Warning)                                                                   \
  V(WeakRef)                                                                   \
  V(ZeroDivisionError)

#define INTRINSIC_CLASS_NAMES(V)                                               \
  INTRINSIC_IMMEDIATE_CLASS_NAMES(V)                                           \
  INTRINSIC_HEAP_CLASS_NAMES(V)

// This enumerates layout ids of intrinsic classes. Notably, the layout of an
// instance of an intrinsic class does not change.
//
// An instance of an intrinsic class that has an immediate representation
// cannot have attributes added.  An instance of an intrinsic class that is heap
// allocated has a predefined number in-object attributes in the base
// instance.  For some of those types, the language forbids adding new
// attributes.  For the types which are permitted to have attributes added,
// these types must include a hidden attribute that indirects to attribute
// storage.
//
// NB: If you add something here make sure you add it to the appropriate macro
// above
enum class LayoutId : word {
  // Immediate objects - note that the RawSmallInt class is also aliased to
  // all even integers less than 32, so that classes of immediate objects can
  // be looked up simply by using the low 5 bits of the immediate value. This
  // implies that all other immediate class ids must be odd.
  kSmallInt = 0,
  kBool = 7,
  kNoneType = 15,
  // there is no class associated with the RawError object type, this is here as
  // a
  // placeholder.
  kError = 23,
  kSmallStr = 31,

  // Heap objects
  kObject = 32,
  kArithmeticError,
  kAssertionError,
  kAttributeError,
  kBaseException,
  kBlockingIOError,
  kBoundMethod,
  kBrokenPipeError,
  kBufferError,
  kBytes,
  kBytesWarning,
  kChildProcessError,
  kClassMethod,
  kCode,
  kComplex,
  kConnectionAbortedError,
  kConnectionError,
  kConnectionRefusedError,
  kConnectionResetError,
  kCoroutine,
  kDeprecationWarning,
  kDict,
  kEOFError,
  kEllipsis,
  kException,
  kFileExistsError,
  kFileNotFoundError,
  kFloat,
  kFloatingPointError,
  kFunction,
  kFutureWarning,
  kGenerator,
  kGeneratorExit,
  kHeapFrame,
  kImportError,
  kImportWarning,
  kIndentationError,
  kIndexError,
  kInt,
  kInterruptedError,
  kIsADirectoryError,
  kKeyError,
  kKeyboardInterrupt,
  kLargeInt,
  kLargeStr,
  kLayout,
  kList,
  kListIterator,
  kLookupError,
  kMemoryError,
  kModule,
  kModuleNotFoundError,
  kNameError,
  kNotADirectoryError,
  kNotImplemented,
  kNotImplementedError,
  kOSError,
  kObjectArray,
  kOverflowError,
  kPendingDeprecationWarning,
  kPermissionError,
  kProcessLookupError,
  kProperty,
  kRange,
  kRangeIterator,
  kRecursionError,
  kReferenceError,
  kResourceWarning,
  kRuntimeError,
  kRuntimeWarning,
  kSet,
  kSetIterator,
  kSlice,
  kStaticMethod,
  kStopAsyncIteration,
  kStopIteration,
  kStr,
  kSuper,
  kSyntaxError,
  kSyntaxWarning,
  kSystemError,
  kSystemExit,
  kTabError,
  kTimeoutError,
  kTupleIterator,
  kType,
  kTypeError,
  kUnboundLocalError,
  kUnicodeDecodeError,
  kUnicodeEncodeError,
  kUnicodeError,
  kUnicodeTranslateError,
  kUnicodeWarning,
  kUserWarning,
  kValueCell,
  kValueError,
  kWarning,
  kWeakRef,
  kZeroDivisionError,

  kLastBuiltinId = kZeroDivisionError,
};

// Map from type to its corresponding LayoutId:
// ObjectLayoutId<RawSmallInt>::value == LayoutId::kSmallInt, etc.
template <typename T>
struct ObjectLayoutId;
#define CASE(ty)                                                               \
  template <>                                                                  \
  struct ObjectLayoutId<class Raw##ty> {                                       \
    static constexpr LayoutId value = LayoutId::k##ty;                         \
  };
INTRINSIC_CLASS_NAMES(CASE)
#undef CASE

// Add functionality common to all RawObject subclasses, split into two parts
// since some types manually define cast() but want everything else.
#define RAW_OBJECT_COMMON_NO_CAST(ty)                                          \
  /* TODO(T34683229) The const_cast here is temporary for a migration */       \
  Raw##ty* operator->() const { return const_cast<Raw##ty*>(this); }           \
  DISALLOW_HEAP_ALLOCATION();

#define RAW_OBJECT_COMMON(ty)                                                  \
  RAW_OBJECT_COMMON_NO_CAST(ty)                                                \
  static Raw##ty cast(RawObject object) {                                      \
    DCHECK(object.is##ty(), "invalid cast, expected " #ty);                    \
    return bit_cast<Raw##ty>(object);                                          \
  }

class RawObject {
 public:
  // TODO(bsimmers): Delete this. The default constructor gives you a
  // zero-initialized RawObject, which is equivalent to
  // RawSmallInt::fromWord(0). This behavior can be confusing and surprising, so
  // we should just require all Objects to be explicitly initialized.
  RawObject() = default;

  explicit RawObject(uword raw);

  // Getters and setters.
  uword raw() const;
  bool isObject();
  LayoutId layoutId();

  // Immediate objects
  bool isBool();
  bool isError();
  bool isHeader();
  bool isNoneType();
  bool isSmallInt();
  bool isSmallStr();

  // Heap objects
  bool isBaseException();
  bool isBoundMethod();
  bool isBytes();
  bool isType();
  bool isClassMethod();
  bool isCode();
  bool isComplex();
  bool isCoroutine();
  bool isDict();
  bool isEllipsis();
  bool isException();
  bool isFloat();
  bool isHeapFrame();
  bool isFunction();
  bool isGenerator();
  bool isHeapObject();
  bool isHeapObjectWithLayout(LayoutId layout_id);
  bool isImportError();
  bool isIndexError();
  bool isInstance();
  bool isKeyError();
  bool isLargeInt();
  bool isLargeStr();
  bool isLayout();
  bool isList();
  bool isListIterator();
  bool isLookupError();
  bool isModule();
  bool isModuleNotFoundError();
  bool isNotImplemented();
  bool isNotImplementedError();
  bool isObjectArray();
  bool isProperty();
  bool isRange();
  bool isRangeIterator();
  bool isGeneratorBase();
  bool isRuntimeError();
  bool isSet();
  bool isSetIterator();
  bool isSlice();
  bool isStaticMethod();
  bool isStopIteration();
  bool isSuper();
  bool isSystemExit();
  bool isTupleIterator();
  bool isValueCell();
  bool isWeakRef();

  // superclass objects
  bool isInt();
  bool isStr();

  static bool equals(RawObject lhs, RawObject rhs);

  bool operator==(const RawObject& other) const;
  bool operator!=(const RawObject& other) const;

  // Constants

  // The bottom five bits of immediate objects are used as the class id when
  // indexing into the class table in the runtime.
  static const uword kImmediateClassTableIndexMask = (1 << 5) - 1;

  RAW_OBJECT_COMMON(Object)

 private:
  // Zero-initializing raw_ gives RawSmallInt::fromWord(0).
  uword raw_{};
};

// CastError and OptInt<T> represent the result of a call to RawInt::asInt<T>():
// If error == CastError::None, value contains the result. Otherwise, error
// indicates why the value didn't fit in T.
enum class CastError {
  None,
  Underflow,
  Overflow,
};

template <typename T>
class OptInt {
 public:
  static OptInt valid(T i) { return {i, CastError::None}; }
  static OptInt underflow() { return {0, CastError::Underflow}; }
  static OptInt overflow() { return {0, CastError::Overflow}; }

  T value;
  CastError error;
};

// Generic wrapper around RawSmallInt/RawLargeInt.
class RawInt : public RawObject {
 public:
  // Getters and setters.
  word asWord();
  void* asCPtr();

  // If this fits in T, get its value as a T. If not, indicate what went wrong.
  template <typename T>
  OptInt<T> asInt();

  word compare(RawInt other);

  double floatValue();

  word bitLength();

  bool isNegative();
  bool isPositive();
  bool isZero();

  RAW_OBJECT_COMMON(Int)

  // Indexing into digits
  word digitAt(word index);

  // Number of digits
  word numDigits();
};

// Immediate objects

class RawSmallInt : public RawObject {
 public:
  // Getters and setters.
  word value();
  void* asCPtr();

  // If this fits in T, get its value as a T. If not, indicate what went wrong.
  template <typename T>
  if_signed_t<T, OptInt<T>> asInt();
  template <typename T>
  if_unsigned_t<T, OptInt<T>> asInt();

  // Conversion.
  static RawSmallInt fromWord(word value);
  static constexpr bool isValid(word value) {
    return (value >= kMinValue) && (value <= kMaxValue);
  }

  template <typename T>
  static RawSmallInt fromFunctionPointer(T pointer);

  // Tags.
  static const int kTag = 0;
  static const int kTagSize = 1;
  static const uword kTagMask = (1 << kTagSize) - 1;

  // Constants
  static const word kMinValue = -(1L << (kBitsPerPointer - (kTagSize + 1)));
  static const word kMaxValue = (1L << (kBitsPerPointer - (kTagSize + 1))) - 1;

  RAW_OBJECT_COMMON(SmallInt)
};

enum class ObjectFormat {
  // Arrays that do not contain objects, one per element width
  kDataArray8 = 0,
  kDataArray16 = 1,
  kDataArray32 = 2,
  kDataArray64 = 3,
  kDataArray128 = 4,
  // Arrays that contain objects
  kObjectArray = 5,
  // Instances that do not contain objects
  kDataInstance = 6,
  // Instances that contain objects
  kObjectInstance = 7,
};

/**
 * RawHeader objects
 *
 * Headers are located in first logical word of a heap allocated object and
 * contain metadata related to the object its part of.  A header is not
 * really object that the user will interact with directly.  Nevertheless, we
 * tag them as immediate object.  This allows the runtime to identify the start
 * of an object when scanning the heap.
 *
 * Headers encode the following information
 *
 * Name      Size  Description
 * ----------------------------------------------------------------------------
 * Tag          3   tag for a header object
 * Format       3   enumeration describing the object encoding
 * RawLayout      20   identifier for the layout, allowing 2^20 unique layouts
 * Hash        30   bits to use for an identity hash code
 * Count        8   number of array elements or instance variables
 */
class RawHeader : public RawObject {
 public:
  word hashCode();
  RawHeader withHashCode(word value);

  ObjectFormat format();

  LayoutId layoutId();
  RawHeader withLayoutId(LayoutId layout_id);

  word count();

  bool hasOverflow();

  static RawHeader from(word count, word hash, LayoutId layout_id,
                        ObjectFormat format);

  // Tags.
  static const int kTag = 3;
  static const int kTagSize = 3;
  static const uword kTagMask = (1 << kTagSize) - 1;

  static const int kFormatSize = 3;
  static const int kFormatOffset = 3;
  static const uword kFormatMask = (1 << kFormatSize) - 1;

  static const int kLayoutIdSize = 20;
  static const int kLayoutIdOffset = 6;
  static const uword kLayoutIdMask = (1 << kLayoutIdSize) - 1;

  static const int kHashCodeOffset = 26;
  static const int kHashCodeSize = 30;
  static const uword kHashCodeMask = (1 << kHashCodeSize) - 1U;

  static const int kCountOffset = 56;
  static const int kCountSize = 8;
  static const uword kCountMask = (1 << kCountSize) - 1;

  static const int kCountOverflowFlag = (1 << kCountSize) - 1;
  static const int kCountMax = kCountOverflowFlag - 1;

  static const int kSize = kPointerSize;

  // Constants
  static const word kMaxLayoutId = (1L << (kLayoutIdSize + 1)) - 1;

  RAW_OBJECT_COMMON(Header)
};

class RawBool : public RawObject {
 public:
  // Getters and setters.
  bool value();

  // Singletons
  static RawBool trueObj();
  static RawBool falseObj();

  // Conversion.
  static RawBool fromBool(bool value);
  static RawBool negate(RawObject value);

  // Tags.
  static const int kTag = 7;  // 0b00111
  static const int kTagSize = 5;
  static const uword kTagMask = (1 << kTagSize) - 1;

  RAW_OBJECT_COMMON(Bool)
};

class RawNoneType : public RawObject {
 public:
  // Singletons.
  static RawNoneType object();

  // Tags.
  static const int kTag = 15;  // 0b01111
  static const int kTagSize = 5;
  static const uword kTagMask = (1 << kTagSize) - 1;

  RAW_OBJECT_COMMON(NoneType)
};

// RawError is a special object type, internal to the runtime. It is used to
// signal that an error has occurred inside the runtime or native code, e.g. an
// exception has been thrown.
class RawError : public RawObject {
 public:
  // Singletons.
  static RawError object();

  // Tagging.
  static const int kTag = 23;  // 0b10111
  static const int kTagSize = 5;
  static const uword kTagMask = (1 << kTagSize) - 1;

  RAW_OBJECT_COMMON(Error)
};

// RawSuper class of common string functionality
class RawStr : public RawObject {
 public:
  // Getters and setters.
  byte charAt(word index);
  word length();
  void copyTo(byte* dst, word length);

  // Equality checks.
  word compare(RawObject string);
  bool equals(RawObject that);
  bool equalsCStr(const char* c_str);

  // Conversion to an unescaped C string.  The underlying memory is allocated
  // with malloc and must be freed by the caller.
  char* toCStr();

  RAW_OBJECT_COMMON(Str)
};

class RawSmallStr : public RawObject {
 public:
  // Conversion.
  static RawObject fromCStr(const char* value);
  static RawObject fromBytes(View<byte> data);

  // Tagging.
  static const int kTag = 31;  // 0b11111
  static const int kTagSize = 5;
  static const uword kTagMask = (1 << kTagSize) - 1;

  static const word kMaxLength = kWordSize - 1;

  RAW_OBJECT_COMMON(SmallStr)

 private:
  // Interface methods are private: strings should be manipulated via the
  // RawStr class, which delegates to RawLargeStr/RawSmallStr appropriately.

  // Getters and setters.
  word length();
  byte charAt(word index);
  void copyTo(byte* dst, word length);

  // Conversion to an unescaped C string.  The underlying memory is allocated
  // with malloc and must be freed by the caller.
  char* toCStr();

  friend class Heap;
  friend class RawObject;
  friend class Runtime;
  friend class RawStr;
};

// Heap objects

class RawHeapObject : public RawObject {
 public:
  // Getters and setters.
  uword address();
  uword baseAddress();
  RawHeader header();
  void setHeader(RawHeader header);
  word headerOverflow();
  void setHeaderAndOverflow(word count, word hash, LayoutId id,
                            ObjectFormat format);
  word headerCountOrOverflow();
  word size();

  // Conversion.
  static RawHeapObject fromAddress(uword address);

  // Sizing
  static word headerSize(word count);

  void initialize(word size, RawObject value);

  // Garbage collection.
  bool isRoot();
  bool isForwarding();
  RawObject forward();
  void forwardTo(RawObject object);

  // Tags.
  static const int kTag = 1;
  static const int kTagSize = 2;
  static const uword kTagMask = (1 << kTagSize) - 1;

  static const uword kIsForwarded = static_cast<uword>(-3);

  // RawLayout.
  static const int kHeaderOffset = -kPointerSize;
  static const int kHeaderOverflowOffset = kHeaderOffset - kPointerSize;
  static const int kSize = kHeaderOffset + kPointerSize;

  static const word kMinimumSize = kPointerSize * 2;

  RAW_OBJECT_COMMON(HeapObject)

 protected:
  RawObject instanceVariableAt(word offset);
  void instanceVariableAtPut(word offset, RawObject value);

 private:
  friend class Runtime;
};

class RawBaseException : public RawHeapObject {
 public:
  // Getters and setters.
  RawObject args();
  void setArgs(RawObject args);

  RawObject traceback();
  void setTraceback(RawObject traceback);

  RawObject cause();
  void setCause(RawObject cause);

  RawObject context();
  void setContext(RawObject context);

  static const int kArgsOffset = RawHeapObject::kSize;
  static const int kTracebackOffset = kArgsOffset + kPointerSize;
  static const int kCauseOffset = kTracebackOffset + kPointerSize;
  static const int kContextOffset = kCauseOffset + kPointerSize;
  static const int kSize = kContextOffset + kPointerSize;

  RAW_OBJECT_COMMON(BaseException)
};

class RawException : public RawBaseException {
 public:
  RAW_OBJECT_COMMON(Exception)
};

class RawStopIteration : public RawBaseException {
 public:
  // Getters and setters.
  RawObject value();
  void setValue(RawObject value);

  static const int kValueOffset = RawBaseException::kSize;
  static const int kSize = kValueOffset + kPointerSize;

  RAW_OBJECT_COMMON(StopIteration)
};

class RawSystemExit : public RawBaseException {
 public:
  RawObject code();
  void setCode(RawObject code);

  static const int kCodeOffset = RawBaseException::kSize;
  static const int kSize = kCodeOffset + kPointerSize;

  RAW_OBJECT_COMMON(SystemExit)
};

class RawRuntimeError : public RawException {
 public:
  RAW_OBJECT_COMMON(RuntimeError)
};

class RawNotImplementedError : public RawRuntimeError {
 public:
  RAW_OBJECT_COMMON(NotImplementedError)
};

class RawImportError : public RawException {
 public:
  // Getters and setters
  RawObject msg();
  void setMsg(RawObject msg);

  RawObject name();
  void setName(RawObject name);

  RawObject path();
  void setPath(RawObject path);

  static const int kMsgOffset = RawBaseException::kSize;
  static const int kNameOffset = kMsgOffset + kPointerSize;
  static const int kPathOffset = kNameOffset + kPointerSize;
  static const int kSize = kPathOffset + kPointerSize;

  RAW_OBJECT_COMMON(ImportError)
};

class RawModuleNotFoundError : public RawImportError {
 public:
  RAW_OBJECT_COMMON(ModuleNotFoundError)
};

class RawLookupError : public RawException {
 public:
  RAW_OBJECT_COMMON(LookupError)
};

class RawIndexError : public RawLookupError {
 public:
  RAW_OBJECT_COMMON(IndexError)
};

class RawKeyError : public RawLookupError {
 public:
  RAW_OBJECT_COMMON(KeyError)
};

class RawType : public RawHeapObject {
 public:
  enum Flag : word {
    kBaseExceptionSubclass = 1 << 0,
    kComplexSubclass = 1 << 1,
    kDictSubclass = 1 << 2,
    kFloatSubclass = 1 << 3,
    kIntSubclass = 1 << 4,
    kListSubclass = 1 << 5,
    kSetSubclass = 1 << 6,
    kStopIterationSubclass = 1 << 7,
    kStrSubclass = 1 << 8,
    kSystemExitSubclass = 1 << 9,
    kTupleSubclass = 1 << 10,
    kTypeSubclass = 1 << 11,
    kLast = kTypeSubclass,
  };
  static_assert(Flag::kLast < RawSmallInt::kMaxValue,
                "Flags must be encodable in a RawSmallInt");

  // Getters and setters.
  RawObject instanceLayout();
  void setInstanceLayout(RawObject layout);

  RawObject mro();
  void setMro(RawObject object_array);

  RawObject name();
  void setName(RawObject name);

  RawObject flags();
  void setFlags(RawObject value);
  void setFlag(Flag flag);
  bool hasFlag(Flag flag);

  RawObject dict();
  void setDict(RawObject name);

  // RawInt holding a pointer to a PyTypeObject
  // Only set on classes that were initialized through PyType_Ready
  RawObject extensionType();
  void setExtensionType(RawObject pytype);

  // builtin base related
  RawObject builtinBaseClass();
  void setBuiltinBaseClass(RawObject base);

  bool isIntrinsicOrExtension();

  // Casting.
  static RawType cast(RawObject object);

  // RawLayout.
  static const int kMroOffset = RawHeapObject::kSize;
  static const int kInstanceLayoutOffset = kMroOffset + kPointerSize;
  static const int kNameOffset = kInstanceLayoutOffset + kPointerSize;
  static const int kFlagsOffset = kNameOffset + kPointerSize;
  static const int kDictOffset = kFlagsOffset + kPointerSize;
  static const int kBuiltinBaseClassOffset = kDictOffset + kPointerSize;
  static const int kExtensionTypeOffset =
      kBuiltinBaseClassOffset + kPointerSize;
  static const int kSize = kExtensionTypeOffset + kPointerSize;

  RAW_OBJECT_COMMON_NO_CAST(Type)
};

class RawArray : public RawHeapObject {
 public:
  word length();

  RAW_OBJECT_COMMON_NO_CAST(Array)
};

class RawBytes : public RawArray {
 public:
  // Getters and setters.
  byte byteAt(word index);
  void byteAtPut(word index, byte value);

  // Sizing.
  static word allocationSize(word length);

  RAW_OBJECT_COMMON(Bytes)
};

class RawObjectArray : public RawArray {
 public:
  // Getters and setters.
  RawObject at(word index);
  void atPut(word index, RawObject value);

  // Sizing.
  static word allocationSize(word length);

  void copyTo(RawObject dst);

  void replaceFromWith(word start, RawObject array);

  bool contains(RawObject object);

  RAW_OBJECT_COMMON(ObjectArray)
};

class RawLargeStr : public RawArray {
 public:
  // Sizing.
  static word allocationSize(word length);

  static const int kDataOffset = RawHeapObject::kSize;

  RAW_OBJECT_COMMON(LargeStr)

 private:
  // Interface methods are private: strings should be manipulated via the
  // RawStr class, which delegates to RawLargeStr/RawSmallStr appropriately.

  // Getters and setters.
  byte charAt(word index);
  void copyTo(byte* bytes, word length);

  // Equality checks.
  bool equals(RawObject that);

  // Conversion to an unescaped C string.  The underlying memory is allocated
  // with malloc and must be freed by the caller.
  char* toCStr();

  friend class Heap;
  friend class RawObject;
  friend class Runtime;
  friend class RawStr;
};

// Arbitrary precision signed integer, with 64 bit digits in two's complement
// representation
class RawLargeInt : public RawHeapObject {
 public:
  // Getters and setters.
  word asWord();

  // Return whether or not this RawLargeInt obeys the following invariants:
  // - numDigits() >= 1
  // - The value does not fit in a RawSmallInt
  // - Negative numbers do not have redundant sign-extended digits
  // - Positive numbers do not have redundant zero-extended digits
  bool isValid();

  // RawLargeInt is also used for storing native pointers.
  void* asCPtr();

  // If this fits in T, get its value as a T. If not, indicate what went wrong.
  template <typename T>
  if_signed_t<T, OptInt<T>> asInt();
  template <typename T>
  if_unsigned_t<T, OptInt<T>> asInt();

  // Sizing.
  static word allocationSize(word num_digits);

  // Indexing into digits
  word digitAt(word index);
  void digitAtPut(word index, word digit);

  bool isNegative();
  bool isPositive();

  word bitLength();

  // Number of digits
  word numDigits();

  // RawLayout.
  static const int kValueOffset = RawHeapObject::kSize;
  static const int kSize = kValueOffset + kPointerSize;

  RAW_OBJECT_COMMON(LargeInt)

 private:
  friend class RawInt;
  friend class Runtime;
};

class RawFloat : public RawHeapObject {
 public:
  // Getters and setters.
  double value();

  // Allocation.
  void initialize(double value);

  // Casting.
  static RawFloat cast(RawObject object);

  // RawLayout.
  static const int kValueOffset = RawHeapObject::kSize;
  static const int kSize = kValueOffset + kDoubleSize;

  RAW_OBJECT_COMMON_NO_CAST(Float)
};

class RawComplex : public RawHeapObject {
 public:
  // Getters
  double real();
  double imag();

  // Allocation.
  void initialize(double real, double imag);

  // RawLayout.
  static const int kRealOffset = RawHeapObject::kSize;
  static const int kImagOffset = kRealOffset + kDoubleSize;
  static const int kSize = kImagOffset + kDoubleSize;

  RAW_OBJECT_COMMON(Complex)
};

class RawProperty : public RawHeapObject {
 public:
  // Getters and setters
  RawObject getter();
  void setGetter(RawObject function);

  RawObject setter();
  void setSetter(RawObject function);

  RawObject deleter();
  void setDeleter(RawObject function);

  // RawLayout
  static const int kGetterOffset = RawHeapObject::kSize;
  static const int kSetterOffset = kGetterOffset + kPointerSize;
  static const int kDeleterOffset = kSetterOffset + kPointerSize;
  static const int kSize = kDeleterOffset + kPointerSize;

  RAW_OBJECT_COMMON(Property)
};

class RawRange : public RawHeapObject {
 public:
  // Getters and setters.
  word start();
  void setStart(word value);

  word stop();
  void setStop(word value);

  word step();
  void setStep(word value);

  // RawLayout.
  static const int kStartOffset = RawHeapObject::kSize;
  static const int kStopOffset = kStartOffset + kPointerSize;
  static const int kStepOffset = kStopOffset + kPointerSize;
  static const int kSize = kStepOffset + kPointerSize;

  RAW_OBJECT_COMMON(Range)
};

class RawRangeIterator : public RawHeapObject {
 public:
  // Getters and setters.

  // Bind the iterator to a specific range. The binding should not be changed
  // after creation.
  void setRange(RawObject range);

  // Iteration.
  RawObject next();

  // Number of unconsumed values in the range iterator
  word pendingLength();

  // RawLayout.
  static const int kRangeOffset = RawHeapObject::kSize;
  static const int kCurValueOffset = kRangeOffset + kPointerSize;
  static const int kSize = kCurValueOffset + kPointerSize;

  RAW_OBJECT_COMMON(RangeIterator)

 private:
  static bool isOutOfRange(word cur, word stop, word step);
};

class RawSlice : public RawHeapObject {
 public:
  // Getters and setters.
  RawObject start();
  void setStart(RawObject value);

  RawObject stop();
  void setStop(RawObject value);

  RawObject step();
  void setStep(RawObject value);

  // Returns the correct start, stop, and step word values from this slice
  void unpack(word* start, word* stop, word* step);

  // Takes in the length of a list and the start, stop, and step values
  // Returns the length of the new list and the corrected start and stop values
  static word adjustIndices(word length, word* start, word* stop, word step);

  // RawLayout.
  static const int kStartOffset = RawHeapObject::kSize;
  static const int kStopOffset = kStartOffset + kPointerSize;
  static const int kStepOffset = kStopOffset + kPointerSize;
  static const int kSize = kStepOffset + kPointerSize;

  RAW_OBJECT_COMMON(Slice)
};

class RawStaticMethod : public RawHeapObject {
 public:
  // Getters and setters
  RawObject function();
  void setFunction(RawObject function);

  // RawLayout
  static const int kFunctionOffset = RawHeapObject::kSize;
  static const int kSize = kFunctionOffset + kPointerSize;

  RAW_OBJECT_COMMON(StaticMethod)
};

class RawListIterator : public RawHeapObject {
 public:
  // Getters and setters.
  word index();
  void setIndex(word index);

  RawObject list();
  void setList(RawObject list);

  // Iteration.
  RawObject next();

  // RawLayout.
  static const int kListOffset = RawHeapObject::kSize;
  static const int kIndexOffset = kListOffset + kPointerSize;
  static const int kSize = kIndexOffset + kPointerSize;

  RAW_OBJECT_COMMON(ListIterator)
};

class RawSetIterator : public RawHeapObject {
 public:
  // Getters and setters
  word consumedCount();
  void setConsumedCount(word consumed);

  word index();
  void setIndex(word index);

  RawObject set();
  void setSet(RawObject set);

  // Iteration.
  RawObject next();

  // Number of unconsumed values in the set iterator
  word pendingLength();

  // RawLayout
  static const int kSetOffset = RawHeapObject::kSize;
  static const int kIndexOffset = kSetOffset + kPointerSize;
  static const int kConsumedCountOffset = kIndexOffset + kPointerSize;
  static const int kSize = kConsumedCountOffset + kPointerSize;

  RAW_OBJECT_COMMON(SetIterator)
};

class RawTupleIterator : public RawHeapObject {
 public:
  // Getters and setters.
  word index();

  void setIndex(word index);

  RawObject tuple();

  void setTuple(RawObject tuple);

  // Iteration.
  RawObject next();

  // RawLayout.
  static const int kTupleOffset = RawHeapObject::kSize;
  static const int kIndexOffset = kTupleOffset + kPointerSize;
  static const int kSize = kIndexOffset + kPointerSize;

  RAW_OBJECT_COMMON(TupleIterator)
};

class RawCode : public RawHeapObject {
 public:
  // Matching CPython
  enum Flags {
    OPTIMIZED = 0x0001,
    NEWLOCALS = 0x0002,
    VARARGS = 0x0004,
    VARKEYARGS = 0x0008,
    NESTED = 0x0010,
    GENERATOR = 0x0020,
    NOFREE = 0x0040,  // Shortcut for no free or cell vars
    COROUTINE = 0x0080,
    ITERABLE_COROUTINE = 0x0100,
    ASYNC_GENERATOR = 0x0200,
    SIMPLE_CALL = 0x0400,  // Pyro addition; speeds detection of fast call
  };

  // Getters and setters.
  word argcount();
  void setArgcount(word value);
  word totalArgs();

  word cell2arg();
  void setCell2arg(word value);

  RawObject cellvars();
  void setCellvars(RawObject value);
  word numCellvars();

  RawObject code();
  void setCode(RawObject value);

  RawObject consts();
  void setConsts(RawObject value);

  RawObject filename();
  void setFilename(RawObject value);

  word firstlineno();
  void setFirstlineno(word value);

  word flags();
  void setFlags(word value);

  RawObject freevars();
  void setFreevars(RawObject value);
  word numFreevars();

  word kwonlyargcount();
  void setKwonlyargcount(word value);

  RawObject lnotab();
  void setLnotab(RawObject value);

  RawObject name();
  void setName(RawObject value);

  RawObject names();
  void setNames(RawObject value);

  word nlocals();
  void setNlocals(word value);

  // The total number of variables in this code object: normal locals, cell
  // variables, and free variables.
  word totalVars();

  word stacksize();
  void setStacksize(word value);

  RawObject varnames();
  void setVarnames(RawObject value);

  // RawLayout.
  static const int kArgcountOffset = RawHeapObject::kSize;
  static const int kKwonlyargcountOffset = kArgcountOffset + kPointerSize;
  static const int kNlocalsOffset = kKwonlyargcountOffset + kPointerSize;
  static const int kStacksizeOffset = kNlocalsOffset + kPointerSize;
  static const int kFlagsOffset = kStacksizeOffset + kPointerSize;
  static const int kFirstlinenoOffset = kFlagsOffset + kPointerSize;
  static const int kCodeOffset = kFirstlinenoOffset + kPointerSize;
  static const int kConstsOffset = kCodeOffset + kPointerSize;
  static const int kNamesOffset = kConstsOffset + kPointerSize;
  static const int kVarnamesOffset = kNamesOffset + kPointerSize;
  static const int kFreevarsOffset = kVarnamesOffset + kPointerSize;
  static const int kCellvarsOffset = kFreevarsOffset + kPointerSize;
  static const int kCell2argOffset = kCellvarsOffset + kPointerSize;
  static const int kFilenameOffset = kCell2argOffset + kPointerSize;
  static const int kNameOffset = kFilenameOffset + kPointerSize;
  static const int kLnotabOffset = kNameOffset + kPointerSize;
  static const int kSize = kLnotabOffset + kPointerSize;

  RAW_OBJECT_COMMON(Code)
};

class Frame;
class Thread;

/**
 * A function object.
 *
 * This may contain a user-defined function or a built-in function.
 *
 * RawFunction objects have a set of pre-defined attributes, only some of which
 * are writable outside of the runtime. The full set is defined at
 *
 *     https://docs.python.org/3/reference/datamodel.html
 */
class RawFunction : public RawHeapObject {
 public:
  /**
   * An entry point into a function.
   *
   * The entry point is called with the current thread, the caller's stack
   * frame, and the number of arguments that have been pushed onto the stack.
   */
  using Entry = RawObject (*)(Thread*, Frame*, word);

  // Getters and setters.

  // A dict containing parameter annotations
  RawObject annotations();
  void setAnnotations(RawObject annotations);

  // The code object backing this function or None
  RawObject code();
  void setCode(RawObject code);

  // A tuple of cell objects that contain bindings for the function's free
  // variables. Read-only to user code.
  RawObject closure();
  void setClosure(RawObject closure);

  // A tuple containing default values for arguments with defaults. Read-only
  // to user code.
  RawObject defaults();
  void setDefaults(RawObject defaults);
  bool hasDefaults();

  // The function's docstring
  RawObject doc();
  void setDoc(RawObject doc);

  // Returns the entry to be used when the function is invoked via
  // CALL_FUNCTION
  Entry entry();
  void setEntry(Entry entry);

  // Returns the entry to be used when the function is invoked via
  // CALL_FUNCTION_KW
  Entry entryKw();
  void setEntryKw(Entry entry_kw);

  // Returns the entry to be used when the function is invoked via
  // CALL_FUNCTION_EX
  inline Entry entryEx();
  inline void setEntryEx(Entry entry_ex);

  // The dict that holds this function's global namespace. User-code
  // cannot change this
  RawObject globals();
  void setGlobals(RawObject globals);

  // A dict containing defaults for keyword-only parameters
  RawObject kwDefaults();
  void setKwDefaults(RawObject kw_defaults);

  // The name of the module the function was defined in
  RawObject module();
  void setModule(RawObject module);

  // The function's name
  RawObject name();
  void setName(RawObject name);

  // The function's qualname
  RawObject qualname();
  void setQualname(RawObject qualname);

  // The pre-computed object array provided fast globals access.
  // fastGlobals[arg] == globals[names[arg]]
  RawObject fastGlobals();
  void setFastGlobals(RawObject fast_globals);

  // RawLayout.
  static const int kDocOffset = RawHeapObject::kSize;
  static const int kNameOffset = kDocOffset + kPointerSize;
  static const int kQualnameOffset = kNameOffset + kPointerSize;
  static const int kModuleOffset = kQualnameOffset + kPointerSize;
  static const int kDefaultsOffset = kModuleOffset + kPointerSize;
  static const int kCodeOffset = kDefaultsOffset + kPointerSize;
  static const int kAnnotationsOffset = kCodeOffset + kPointerSize;
  static const int kKwDefaultsOffset = kAnnotationsOffset + kPointerSize;
  static const int kClosureOffset = kKwDefaultsOffset + kPointerSize;
  static const int kGlobalsOffset = kClosureOffset + kPointerSize;
  static const int kEntryOffset = kGlobalsOffset + kPointerSize;
  static const int kEntryKwOffset = kEntryOffset + kPointerSize;
  static const int kEntryExOffset = kEntryKwOffset + kPointerSize;
  static const int kFastGlobalsOffset = kEntryExOffset + kPointerSize;
  static const int kSize = kFastGlobalsOffset + kPointerSize;

  RAW_OBJECT_COMMON(Function)
};

class RawInstance : public RawHeapObject {
 public:
  // Sizing.
  static word allocationSize(word num_attributes);

  RAW_OBJECT_COMMON(Instance)
};

class RawModule : public RawHeapObject {
 public:
  // Setters and getters.
  RawObject name();
  void setName(RawObject name);

  RawObject dict();
  void setDict(RawObject dict);

  // Contains the numeric address of mode definition object for C-API modules or
  // zero if the module was not defined through the C-API.
  RawObject def();
  void setDef(RawObject def);

  // RawLayout.
  static const int kNameOffset = RawHeapObject::kSize;
  static const int kDictOffset = kNameOffset + kPointerSize;
  static const int kDefOffset = kDictOffset + kPointerSize;
  static const int kSize = kDefOffset + kPointerSize;

  RAW_OBJECT_COMMON(Module)
};

class RawNotImplemented : public RawHeapObject {
 public:
  // RawLayout
  // kPaddingOffset is not used, but the GC expects the object to be
  // at least one word.
  static const int kPaddingOffset = RawHeapObject::kSize;
  static const int kSize = kPaddingOffset + kPointerSize;

  RAW_OBJECT_COMMON(NotImplemented)
};

/**
 * A simple dict that uses open addressing and linear probing.
 *
 * RawLayout:
 *
 *   [RawType pointer]
 *   [NumItems     ] - Number of items currently in the dict
 *   [Items        ] - Pointer to an RawObjectArray that stores the underlying
 * data.
 *
 * RawDict entries are stored in buckets as a triple of (hash, key, value).
 * Empty buckets are stored as (RawNoneType, RawNoneType, RawNoneType).
 * Tombstone buckets are stored as (RawNoneType, <not RawNoneType>, <Any>).
 *
 */
class RawDict : public RawHeapObject {
 public:
  class Bucket;

  // Getters and setters.
  // The RawObjectArray backing the dict
  RawObject data();
  void setData(RawObject data);

  // Number of items currently in the dict
  word numItems();
  void setNumItems(word num_items);

  // RawLayout.
  static const int kNumItemsOffset = RawHeapObject::kSize;
  static const int kDataOffset = kNumItemsOffset + kPointerSize;
  static const int kSize = kDataOffset + kPointerSize;

  RAW_OBJECT_COMMON(Dict)
};

// Helper class for manipulating buckets in the RawObjectArray that backs the
// dict
class RawDict::Bucket {
 public:
  // none of these operations do bounds checking on the backing array
  static word getIndex(RawObjectArray data, RawObject hash) {
    word nbuckets = data->length() / kNumPointers;
    DCHECK(Utils::isPowerOfTwo(nbuckets), "%ld is not a power of 2", nbuckets);
    word value = RawSmallInt::cast(hash)->value();
    return (value & (nbuckets - 1)) * kNumPointers;
  }

  static bool hasKey(RawObjectArray data, word index, RawObject that_key) {
    return !hash(data, index)->isNoneType() &&
           RawObject::equals(key(data, index), that_key);
  }

  static RawObject hash(RawObjectArray data, word index) {
    return data->at(index + kHashOffset);
  }

  static bool isEmpty(RawObjectArray data, word index) {
    return hash(data, index)->isNoneType() && key(data, index)->isNoneType();
  }

  static bool isTombstone(RawObjectArray data, word index) {
    return hash(data, index)->isNoneType() && !key(data, index)->isNoneType();
  }

  static bool isFilled(RawObjectArray data, word index) {
    return !hash(data, index)->isNoneType();
  }

  static RawObject key(RawObjectArray data, word index) {
    return data->at(index + kKeyOffset);
  }

  static void set(RawObjectArray data, word index, RawObject hash,
                  RawObject key, RawObject value) {
    data->atPut(index + kHashOffset, hash);
    data->atPut(index + kKeyOffset, key);
    data->atPut(index + kValueOffset, value);
  }

  static void setTombstone(RawObjectArray data, word index) {
    set(data, index, RawNoneType::object(), RawError::object(),
        RawNoneType::object());
  }

  static RawObject value(RawObjectArray data, word index) {
    return data->at(index + kValueOffset);
  }

  // RawLayout.
  static const word kHashOffset = 0;
  static const word kKeyOffset = kHashOffset + 1;
  static const word kValueOffset = kKeyOffset + 1;
  static const word kNumPointers = kValueOffset + 1;

 private:
  DISALLOW_HEAP_ALLOCATION();
};

/**
 * A simple set implementation.
 */
class RawSet : public RawHeapObject {
 public:
  class Bucket;

  // Getters and setters.
  // The RawObjectArray backing the set
  RawObject data();
  void setData(RawObject data);

  // Number of items currently in the set
  word numItems();
  void setNumItems(word num_items);

  // RawLayout.
  static const int kNumItemsOffset = RawHeapObject::kSize;
  static const int kDataOffset = kNumItemsOffset + kPointerSize;
  static const int kSize = kDataOffset + kPointerSize;

  RAW_OBJECT_COMMON(Set)
};

// Helper class for manipulating buckets in the RawObjectArray that backs the
// set
class RawSet::Bucket {
 public:
  // none of these operations do bounds checking on the backing array
  static word getIndex(RawObjectArray data, RawObject hash) {
    word nbuckets = data->length() / kNumPointers;
    DCHECK(Utils::isPowerOfTwo(nbuckets), "%ld not a power of 2", nbuckets);
    word value = RawSmallInt::cast(hash)->value();
    return (value & (nbuckets - 1)) * kNumPointers;
  }

  static RawObject hash(RawObjectArray data, word index) {
    return data->at(index + kHashOffset);
  }

  static bool hasKey(RawObjectArray data, word index, RawObject that_key) {
    return !hash(data, index)->isNoneType() &&
           RawObject::equals(key(data, index), that_key);
  }

  static bool isEmpty(RawObjectArray data, word index) {
    return hash(data, index)->isNoneType() && key(data, index)->isNoneType();
  }

  static bool isTombstone(RawObjectArray data, word index) {
    return hash(data, index)->isNoneType() && !key(data, index)->isNoneType();
  }

  static bool isFilled(RawObjectArray data, word index) {
    return !hash(data, index)->isNoneType();
  }

  static RawObject key(RawObjectArray data, word index) {
    return data->at(index + kKeyOffset);
  }

  static void set(RawObjectArray data, word index, RawObject hash,
                  RawObject key) {
    data->atPut(index + kHashOffset, hash);
    data->atPut(index + kKeyOffset, key);
  }

  static void setTombstone(RawObjectArray data, word index) {
    set(data, index, RawNoneType::object(), RawError::object());
  }

  // RawLayout.
  static const word kHashOffset = 0;
  static const word kKeyOffset = kHashOffset + 1;
  static const word kNumPointers = kKeyOffset + 1;

 private:
  DISALLOW_HEAP_ALLOCATION();
};

/**
 * A growable array
 *
 * RawLayout:
 *
 *   [RawType pointer]
 *   [Length       ] - Number of elements currently in the list
 *   [Elems        ] - Pointer to an RawObjectArray that contains list elements
 */
class RawList : public RawHeapObject {
 public:
  // Getters and setters.
  RawObject at(word index);
  void atPut(word index, RawObject value);
  RawObject items();
  void setItems(RawObject new_items);
  word numItems();
  void setNumItems(word num_items);

  // Return the total number of elements that may be held without growing the
  // list
  word capacity();

  // Casting.
  static RawList cast(RawObject object);

  // RawLayout.
  static const int kItemsOffset = RawHeapObject::kSize;
  static const int kAllocatedOffset = kItemsOffset + kPointerSize;
  static const int kSize = kAllocatedOffset + kPointerSize;

  RAW_OBJECT_COMMON_NO_CAST(List)
};

class RawValueCell : public RawHeapObject {
 public:
  // Getters and setters
  RawObject value();
  void setValue(RawObject object);

  bool isUnbound();
  void makeUnbound();

  // RawLayout.
  static const int kValueOffset = RawHeapObject::kSize;
  static const int kSize = kValueOffset + kPointerSize;

  RAW_OBJECT_COMMON(ValueCell)
};

class RawEllipsis : public RawHeapObject {
 public:
  // RawLayout
  // kPaddingOffset is not used, but the GC expects the object to be
  // at least one word.
  static const int kPaddingOffset = RawHeapObject::kSize;
  static const int kSize = kPaddingOffset + kPointerSize;

  RAW_OBJECT_COMMON(Ellipsis)
};

class RawWeakRef : public RawHeapObject {
 public:
  // Getters and setters

  // The object weakly referred to by this instance.
  RawObject referent();
  void setReferent(RawObject referent);

  // A callable object invoked with the referent as an argument when the
  // referent is deemed to be "near death" and only reachable through this weak
  // reference.
  RawObject callback();
  void setCallback(RawObject callable);

  // Singly linked list of weak reference objects.  This field is used during
  // garbage collection to represent the set of weak references that had been
  // discovered by the initial trace with an otherwise unreachable referent.
  RawObject link();
  void setLink(RawObject reference);

  static void enqueueReference(RawObject reference, RawObject* list);
  static RawObject dequeueReference(RawObject* list);
  static RawObject spliceQueue(RawObject tail1, RawObject tail2);

  // RawLayout.
  static const int kReferentOffset = RawHeapObject::kSize;
  static const int kCallbackOffset = kReferentOffset + kPointerSize;
  static const int kLinkOffset = kCallbackOffset + kPointerSize;
  static const int kSize = kLinkOffset + kPointerSize;

  RAW_OBJECT_COMMON(WeakRef)
};

/**
 * A RawBoundMethod binds a RawFunction and its first argument (called `self`).
 *
 * These are typically created as a temporary object during a method call,
 * though they may be created and passed around freely.
 *
 * Consider the following snippet of python code:
 *
 *   class Foo:
 *     def bar(self):
 *       return self
 *   f = Foo()
 *   f.bar()
 *
 * The Python 3.6 bytecode produced for the line `f.bar()` is:
 *
 *   LOAD_FAST                0 (f)
 *   LOAD_ATTR                1 (bar)
 *   CALL_FUNCTION            0
 *
 * The LOAD_ATTR for `f.bar` creates a `RawBoundMethod`, which is then called
 * directly by the subsequent CALL_FUNCTION opcode.
 */
class RawBoundMethod : public RawHeapObject {
 public:
  // Getters and setters

  // The function to which "self" is bound
  RawObject function();
  void setFunction(RawObject function);

  // The instance of "self" being bound
  RawObject self();
  void setSelf(RawObject self);

  // RawLayout
  static const int kFunctionOffset = RawHeapObject::kSize;
  static const int kSelfOffset = kFunctionOffset + kPointerSize;
  static const int kSize = kSelfOffset + kPointerSize;

  RAW_OBJECT_COMMON(BoundMethod)
};

class RawClassMethod : public RawHeapObject {
 public:
  // Getters and setters
  RawObject function();
  void setFunction(RawObject function);

  // RawLayout
  static const int kFunctionOffset = RawHeapObject::kSize;
  static const int kSize = kFunctionOffset + kPointerSize;

  RAW_OBJECT_COMMON(ClassMethod)
};

/**
 * A RawLayout describes the in-memory shape of an instance.
 *
 * RawInstance attributes are split into two classes: in-object attributes,
 * which exist directly in the instance, and overflow attributes, which are
 * stored in an object array pointed to by the last word of the instance.
 * Graphically, this looks like:
 *
 *   RawInstance                                   RawObjectArray
 *   +---------------------------+     +------->+--------------------------+
 *   | First in-object attribute |     |        | First overflow attribute |
 *   +---------------------------+     |        +--------------------------+
 *   |            ...            |     |        |           ...            |
 *   +---------------------------+     |        +--------------------------+
 *   | Last in-object attribute  |     |        | Last overflow attribute  |
 *   +---------------------------+     |        +--------------------------+
 *   | Overflow Attributes       +-----+
 *   +---------------------------+
 *
 * Each instance is associated with a layout (whose id is stored in the header
 * word). The layout acts as a roadmap for the instance; it describes where to
 * find each attribute.
 *
 * In general, instances of the same class will have the same shape. Idiomatic
 * Python typically initializes attributes in the same order for instances of
 * the same class. Ideally, we would be able to share the same concrete
 * RawLayout between two instances of the same shape. This both reduces memory
 * overhead and enables effective caching of attribute location.
 *
 * To achieve structural sharing, layouts form an immutable DAG. Every class
 * has a root layout that contains only in-object attributes. When an instance
 * is created, it is assigned the root layout of its class. When a shape
 * altering mutation to the instance occurs (e.g. adding an attribute), the
 * current layout is searched for a corresponding edge. If such an edge exists,
 * it is followed and the instance is assigned the resulting layout. If there
 * is no such edge, a new layout is created, an edge is inserted between
 * the two layouts, and the instance is assigned the new layout.
 *
 */
class RawLayout : public RawHeapObject {
 public:
  // Getters and setters.
  LayoutId id();
  void setId(LayoutId id);

  // Returns the class whose instances are described by this layout
  RawObject describedClass();
  void setDescribedClass(RawObject type);

  // RawSet the number of in-object attributes that may be stored on an instance
  // described by this layout.
  //
  // N.B. - This will always be larger than or equal to the length of the
  // RawObjectArray returned by inObjectAttributes().
  void setNumInObjectAttributes(word count);
  word numInObjectAttributes();

  // Returns an RawObjectArray describing the attributes stored directly in
  // in the instance.
  //
  // Each item in the object array is a two element tuple. Each tuple is
  // composed of the following elements, in order:
  //
  //   1. The attribute name (RawStr)
  //   2. The attribute info (AttributeInfo)
  RawObject inObjectAttributes();
  void setInObjectAttributes(RawObject attributes);

  // Returns an RawObjectArray describing the attributes stored in the overflow
  // array of the instance.
  //
  // Each item in the object array is a two element tuple. Each tuple is
  // composed of the following elements, in order:
  //
  //   1. The attribute name (RawStr)
  //   2. The attribute info (AttributeInfo)
  RawObject overflowAttributes();
  void setOverflowAttributes(RawObject attributes);

  // Returns a flattened list of tuples. Each tuple is composed of the
  // following elements, in order:
  //
  //   1. The attribute name (RawStr)
  //   2. The layout that would result if an attribute with that name
  //      was added.
  RawObject additions();
  void setAdditions(RawObject additions);

  // Returns a flattened list of tuples. Each tuple is composed of the
  // following elements, in order:
  //
  //   1. The attribute name (RawStr)
  //   2. The layout that would result if an attribute with that name
  //      was deleted.
  RawObject deletions();
  void setDeletions(RawObject deletions);

  // Returns the number of words in an instance described by this layout,
  // including the overflow array.
  word instanceSize();
  void setInstanceSize(word size);

  // Return the offset, in bytes, of the overflow slot
  word overflowOffset();

  // RawLayout
  static const int kDescribedClassOffset = RawHeapObject::kSize;
  static const int kInObjectAttributesOffset =
      kDescribedClassOffset + kPointerSize;
  static const int kOverflowAttributesOffset =
      kInObjectAttributesOffset + kPointerSize;
  static const int kAdditionsOffset = kOverflowAttributesOffset + kPointerSize;
  static const int kDeletionsOffset = kAdditionsOffset + kPointerSize;
  static const int kInstanceSizeOffset = kDeletionsOffset + kPointerSize;
  static const int kOverflowOffsetOffset = kInstanceSizeOffset + kPointerSize;
  static const int kNumInObjectAttributesOffset =
      kOverflowOffsetOffset + kPointerSize;
  static const int kSize = kNumInObjectAttributesOffset + kPointerSize;

  RAW_OBJECT_COMMON(Layout)

 private:
  void setOverflowOffset(word offset);
};

class RawSuper : public RawHeapObject {
 public:
  // getters and setters
  RawObject type();
  void setType(RawObject tp);
  RawObject object();
  void setObject(RawObject obj);
  RawObject objectType();
  void setObjectType(RawObject tp);

  // RawLayout
  static const int kTypeOffset = RawHeapObject::kSize;
  static const int kObjectOffset = kTypeOffset + kPointerSize;
  static const int kObjectTypeOffset = kObjectOffset + kPointerSize;
  static const int kSize = kObjectTypeOffset + kPointerSize;

  RAW_OBJECT_COMMON(Super)
};

/**
 * Base class containing functionality needed by all objects representing a
 * suspended execution frame: RawGenerator, RawCoroutine, and AsyncGenerator.
 */
class RawGeneratorBase : public RawHeapObject {
 public:
  // Get or set the RawHeapFrame embedded in this RawGeneratorBase.
  RawObject heapFrame();
  void setHeapFrame(RawObject obj);

  // RawLayout.
  static const int kFrameOffset = RawHeapObject::kSize;
  static const int kIsRunningOffset = kFrameOffset + kPointerSize;
  static const int kCodeOffset = kIsRunningOffset + kPointerSize;
  static const int kSize = kCodeOffset + kPointerSize;

  RAW_OBJECT_COMMON(GeneratorBase)
};

class RawGenerator : public RawGeneratorBase {
 public:
  static const int kYieldFromOffset = RawGeneratorBase::kSize;
  static const int kSize = kYieldFromOffset + kPointerSize;

  RAW_OBJECT_COMMON(Generator)
};

class RawCoroutine : public RawGeneratorBase {
 public:
  // RawLayout.
  static const int kAwaitOffset = RawGeneratorBase::kSize;
  static const int kOriginOffset = kAwaitOffset + kPointerSize;
  static const int kSize = kOriginOffset + kPointerSize;

  RAW_OBJECT_COMMON(Coroutine)
};

// RawObject

inline RawObject::RawObject(uword raw) : raw_{raw} {}

inline uword RawObject::raw() const { return raw_; }

inline bool RawObject::isObject() { return true; }

inline LayoutId RawObject::layoutId() {
  if (isHeapObject()) {
    return RawHeapObject::cast(*this)->header()->layoutId();
  }
  if (isSmallInt()) {
    return LayoutId::kSmallInt;
  }
  return static_cast<LayoutId>(raw() & kImmediateClassTableIndexMask);
}

inline bool RawObject::isType() {
  return isHeapObjectWithLayout(LayoutId::kType);
}

inline bool RawObject::isClassMethod() {
  return isHeapObjectWithLayout(LayoutId::kClassMethod);
}

inline bool RawObject::isSmallInt() {
  return (raw() & RawSmallInt::kTagMask) == RawSmallInt::kTag;
}

inline bool RawObject::isSmallStr() {
  return (raw() & RawSmallStr::kTagMask) == RawSmallStr::kTag;
}

inline bool RawObject::isHeader() {
  return (raw() & RawHeader::kTagMask) == RawHeader::kTag;
}

inline bool RawObject::isBool() {
  return (raw() & RawBool::kTagMask) == RawBool::kTag;
}

inline bool RawObject::isNoneType() {
  return (raw() & RawNoneType::kTagMask) == RawNoneType::kTag;
}

inline bool RawObject::isError() {
  return (raw() & RawError::kTagMask) == RawError::kTag;
}

inline bool RawObject::isHeapObject() {
  return (raw() & RawHeapObject::kTagMask) == RawHeapObject::kTag;
}

inline bool RawObject::isHeapObjectWithLayout(LayoutId layout_id) {
  if (!isHeapObject()) {
    return false;
  }
  return RawHeapObject::cast(*this)->header()->layoutId() == layout_id;
}

inline bool RawObject::isLayout() {
  return isHeapObjectWithLayout(LayoutId::kLayout);
}

inline bool RawObject::isBaseException() {
  return isHeapObjectWithLayout(LayoutId::kBaseException);
}

inline bool RawObject::isException() {
  return isHeapObjectWithLayout(LayoutId::kException);
}

inline bool RawObject::isBoundMethod() {
  return isHeapObjectWithLayout(LayoutId::kBoundMethod);
}

inline bool RawObject::isBytes() {
  return isHeapObjectWithLayout(LayoutId::kBytes);
}

inline bool RawObject::isObjectArray() {
  return isHeapObjectWithLayout(LayoutId::kObjectArray);
}

inline bool RawObject::isCode() {
  return isHeapObjectWithLayout(LayoutId::kCode);
}

inline bool RawObject::isComplex() {
  return isHeapObjectWithLayout(LayoutId::kComplex);
}

inline bool RawObject::isCoroutine() {
  return isHeapObjectWithLayout(LayoutId::kCoroutine);
}

inline bool RawObject::isLargeStr() {
  return isHeapObjectWithLayout(LayoutId::kLargeStr);
}

inline bool RawObject::isFunction() {
  return isHeapObjectWithLayout(LayoutId::kFunction);
}

inline bool RawObject::isInstance() {
  if (!isHeapObject()) {
    return false;
  }
  return RawHeapObject::cast(*this)->header()->layoutId() >
         LayoutId::kLastBuiltinId;
}

inline bool RawObject::isKeyError() {
  return isHeapObjectWithLayout(LayoutId::kKeyError);
}

inline bool RawObject::isDict() {
  return isHeapObjectWithLayout(LayoutId::kDict);
}

inline bool RawObject::isFloat() {
  return isHeapObjectWithLayout(LayoutId::kFloat);
}

inline bool RawObject::isHeapFrame() {
  return isHeapObjectWithLayout(LayoutId::kHeapFrame);
}

inline bool RawObject::isSet() {
  return isHeapObjectWithLayout(LayoutId::kSet);
}

inline bool RawObject::isSetIterator() {
  return isHeapObjectWithLayout(LayoutId::kSetIterator);
}

inline bool RawObject::isSuper() {
  return isHeapObjectWithLayout(LayoutId::kSuper);
}

inline bool RawObject::isModule() {
  return isHeapObjectWithLayout(LayoutId::kModule);
}

inline bool RawObject::isList() {
  return isHeapObjectWithLayout(LayoutId::kList);
}

inline bool RawObject::isListIterator() {
  return isHeapObjectWithLayout(LayoutId::kListIterator);
}

inline bool RawObject::isLookupError() {
  return isHeapObjectWithLayout(LayoutId::kLookupError);
}

inline bool RawObject::isValueCell() {
  return isHeapObjectWithLayout(LayoutId::kValueCell);
}

inline bool RawObject::isEllipsis() {
  return isHeapObjectWithLayout(LayoutId::kEllipsis);
}

inline bool RawObject::isGenerator() {
  return isHeapObjectWithLayout(LayoutId::kGenerator);
}

inline bool RawObject::isLargeInt() {
  return isHeapObjectWithLayout(LayoutId::kLargeInt);
}

inline bool RawObject::isInt() {
  return isSmallInt() || isLargeInt() || isBool();
}

inline bool RawObject::isNotImplemented() {
  return isHeapObjectWithLayout(LayoutId::kNotImplemented);
}

inline bool RawObject::isNotImplementedError() {
  return isHeapObjectWithLayout(LayoutId::kNotImplementedError);
}

inline bool RawObject::isProperty() {
  return isHeapObjectWithLayout(LayoutId::kProperty);
}

inline bool RawObject::isRange() {
  return isHeapObjectWithLayout(LayoutId::kRange);
}

inline bool RawObject::isRangeIterator() {
  return isHeapObjectWithLayout(LayoutId::kRangeIterator);
}

inline bool RawObject::isGeneratorBase() {
  return isGenerator() || isCoroutine();
}

inline bool RawObject::isRuntimeError() {
  return isHeapObjectWithLayout(LayoutId::kRuntimeError);
}

inline bool RawObject::isSlice() {
  return isHeapObjectWithLayout(LayoutId::kSlice);
}

inline bool RawObject::isStaticMethod() {
  return isHeapObjectWithLayout(LayoutId::kStaticMethod);
}

inline bool RawObject::isStopIteration() {
  return isHeapObjectWithLayout(LayoutId::kStopIteration);
}

inline bool RawObject::isStr() { return isSmallStr() || isLargeStr(); }

inline bool RawObject::isSystemExit() {
  return isHeapObjectWithLayout(LayoutId::kSystemExit);
}

inline bool RawObject::isTupleIterator() {
  return isHeapObjectWithLayout(LayoutId::kTupleIterator);
}

inline bool RawObject::isImportError() {
  return isHeapObjectWithLayout(LayoutId::kImportError);
}

inline bool RawObject::isIndexError() {
  return isHeapObjectWithLayout(LayoutId::kIndexError);
}

inline bool RawObject::isWeakRef() {
  return isHeapObjectWithLayout(LayoutId::kWeakRef);
}

inline bool RawObject::isModuleNotFoundError() {
  return isHeapObjectWithLayout(LayoutId::kModuleNotFoundError);
}

inline bool RawObject::equals(RawObject lhs, RawObject rhs) {
  return (lhs == rhs) ||
         (lhs->isLargeStr() && RawLargeStr::cast(lhs)->equals(rhs));
}

inline bool RawObject::operator==(const RawObject& other) const {
  return raw() == other.raw();
}

inline bool RawObject::operator!=(const RawObject& other) const {
  return !operator==(other);
}

// RawInt

inline word RawInt::asWord() {
  if (isSmallInt()) {
    return RawSmallInt::cast(*this)->value();
  }
  return RawLargeInt::cast(*this)->asWord();
}

inline void* RawInt::asCPtr() {
  if (isSmallInt()) {
    return RawSmallInt::cast(*this)->asCPtr();
  }
  return RawLargeInt::cast(*this)->asCPtr();
}

template <typename T>
OptInt<T> RawInt::asInt() {
  if (isSmallInt()) return RawSmallInt::cast(*this)->asInt<T>();
  return RawLargeInt::cast(*this)->asInt<T>();
}

inline word RawInt::compare(RawInt that) {
  if (this->isSmallInt() && that->isSmallInt()) {
    return this->asWord() - that->asWord();
  }
  if (this->isNegative() != that->isNegative()) {
    return this->isNegative() ? -1 : 1;
  }

  word left_digits = this->numDigits();
  word right_digits = that->numDigits();

  if (left_digits > right_digits) {
    return 1;
  }
  if (left_digits < right_digits) {
    return -1;
  }
  for (word i = left_digits - 1; i >= 0; i--) {
    word left_digit = this->digitAt(i), right_digit = that->digitAt(i);
    if (left_digit > right_digit) {
      return 1;
    }
    if (left_digit < right_digit) {
      return -1;
    }
  }
  return 0;
}

inline double RawInt::floatValue() {
  if (isSmallInt()) {
    return static_cast<double>(asWord());
  }
  if (isBool()) {
    return RawBool::cast(*this) == RawBool::trueObj() ? 1.0 : 0.0;
  }
  RawLargeInt large_int = RawLargeInt::cast(*this);
  if (large_int->numDigits() == 1) {
    return static_cast<double>(asWord());
  }
  // TODO(T30610701): Handle arbitrary precision LargeInts
  UNIMPLEMENTED("LargeInts with > 1 digit");
}

inline word RawInt::bitLength() {
  if (isSmallInt()) {
    uword self =
        static_cast<uword>(std::abs(RawSmallInt::cast(*this)->value()));
    return Utils::highestBit(self);
  }
  if (isBool()) {
    return RawBool::cast(*this) == RawBool::trueObj() ? 1 : 0;
  }
  return RawLargeInt::cast(*this)->bitLength();
}

inline bool RawInt::isPositive() {
  if (isSmallInt()) {
    return RawSmallInt::cast(*this)->value() > 0;
  }
  if (isBool()) {
    return RawBool::cast(*this) == RawBool::trueObj();
  }
  return RawLargeInt::cast(*this)->isPositive();
}

inline bool RawInt::isNegative() {
  if (isSmallInt()) {
    return RawSmallInt::cast(*this)->value() < 0;
  }
  if (isBool()) {
    return false;
  }
  return RawLargeInt::cast(*this)->isNegative();
}

inline bool RawInt::isZero() {
  if (isSmallInt()) {
    return RawSmallInt::cast(*this)->value() == 0;
  }
  if (isBool()) {
    return RawBool::cast(*this) == RawBool::falseObj();
  }
  // A RawLargeInt can never be zero
  DCHECK(isLargeInt(), "RawObject must be a RawLargeInt");
  return false;
}

inline word RawInt::numDigits() {
  if (isSmallInt() || isBool()) {
    return 1;
  }
  return RawLargeInt::cast(*this)->numDigits();
}

inline word RawInt::digitAt(word index) {
  if (isSmallInt()) {
    DCHECK(index == 0, "RawSmallInt digit index out of bounds");
    return RawSmallInt::cast(*this)->value();
  }
  if (isBool()) {
    DCHECK(index == 0, "RawBool digit index out of bounds");
    return RawBool::cast(*this) == RawBool::trueObj() ? 1 : 0;
  }
  return RawLargeInt::cast(*this)->digitAt(index);
}

// RawSmallInt

inline word RawSmallInt::value() {
  return static_cast<word>(raw()) >> kTagSize;
}

inline void* RawSmallInt::asCPtr() { return reinterpret_cast<void*>(value()); }

template <typename T>
if_signed_t<T, OptInt<T>> RawSmallInt::asInt() {
  static_assert(sizeof(T) <= sizeof(word), "T must not be larger than word");

  auto const value = this->value();
  if (value > std::numeric_limits<T>::max()) return OptInt<T>::overflow();
  if (value < std::numeric_limits<T>::min()) return OptInt<T>::underflow();
  return OptInt<T>::valid(value);
}

template <typename T>
if_unsigned_t<T, OptInt<T>> RawSmallInt::asInt() {
  static_assert(sizeof(T) <= sizeof(word), "T must not be larger than word");
  auto const max = std::numeric_limits<T>::max();
  auto const value = this->value();

  if (value < 0) return OptInt<T>::underflow();
  if (max >= RawSmallInt::kMaxValue || static_cast<uword>(value) <= max) {
    return OptInt<T>::valid(value);
  }
  return OptInt<T>::overflow();
}

inline RawSmallInt RawSmallInt::fromWord(word value) {
  DCHECK(RawSmallInt::isValid(value), "invalid cast");
  return cast(RawObject{static_cast<uword>(value) << kTagSize});
}

template <typename T>
inline RawSmallInt RawSmallInt::fromFunctionPointer(T pointer) {
  // The bit pattern for a function pointer object must be indistinguishable
  // from that of a small integer object.
  return cast(RawObject{reinterpret_cast<uword>(pointer)});
}

// RawSmallStr

inline word RawSmallStr::length() { return (raw() >> kTagSize) & kMaxLength; }

inline byte RawSmallStr::charAt(word index) {
  DCHECK_INDEX(index, length());
  return raw() >> (kBitsPerByte * (index + 1));
}

inline void RawSmallStr::copyTo(byte* dst, word length) {
  DCHECK_BOUND(length, this->length());
  for (word i = 0; i < length; ++i) {
    *dst++ = charAt(i);
  }
}

// RawHeader

inline word RawHeader::count() {
  return static_cast<word>((raw() >> kCountOffset) & kCountMask);
}

inline bool RawHeader::hasOverflow() { return count() == kCountOverflowFlag; }

inline word RawHeader::hashCode() {
  return static_cast<word>((raw() >> kHashCodeOffset) & kHashCodeMask);
}

inline RawHeader RawHeader::withHashCode(word value) {
  auto header = raw();
  header &= ~(kHashCodeMask << kHashCodeOffset);
  header |= (value & kHashCodeMask) << kHashCodeOffset;
  return cast(RawObject{header});
}

inline LayoutId RawHeader::layoutId() {
  return static_cast<LayoutId>((raw() >> kLayoutIdOffset) & kLayoutIdMask);
}

inline RawHeader RawHeader::withLayoutId(LayoutId layout_id) {
  DCHECK_BOUND(static_cast<word>(layout_id), kMaxLayoutId);
  auto header = raw();
  header &= ~(kLayoutIdMask << kLayoutIdOffset);
  header |= (static_cast<word>(layout_id) & kLayoutIdMask) << kLayoutIdOffset;
  return cast(RawObject{header});
}

inline ObjectFormat RawHeader::format() {
  return static_cast<ObjectFormat>((raw() >> kFormatOffset) & kFormatMask);
}

inline RawHeader RawHeader::from(word count, word hash, LayoutId id,
                                 ObjectFormat format) {
  DCHECK(
      (count >= 0) && ((count <= kCountMax) || (count == kCountOverflowFlag)),
      "bounds violation, %ld not in 0..%d", count, kCountMax);
  uword result = RawHeader::kTag;
  result |= ((count > kCountMax) ? kCountOverflowFlag : count) << kCountOffset;
  result |= hash << kHashCodeOffset;
  result |= static_cast<uword>(id) << kLayoutIdOffset;
  result |= static_cast<uword>(format) << kFormatOffset;
  return cast(RawObject{result});
}

// None

inline RawNoneType RawNoneType::object() {
  return bit_cast<RawNoneType>(static_cast<uword>(kTag));
}

// RawError

inline RawError RawError::object() {
  return bit_cast<RawError>(static_cast<uword>(kTag));
}

// RawBool

inline RawBool RawBool::trueObj() { return fromBool(true); }

inline RawBool RawBool::falseObj() { return fromBool(false); }

inline RawBool RawBool::negate(RawObject value) {
  DCHECK(value->isBool(), "not a boolean instance");
  return (value == trueObj()) ? falseObj() : trueObj();
}

inline RawBool RawBool::fromBool(bool value) {
  return cast(RawObject{(static_cast<uword>(value) << kTagSize) | kTag});
}

inline bool RawBool::value() { return (raw() >> kTagSize) ? true : false; }

// RawHeapObject

inline uword RawHeapObject::address() { return raw() - RawHeapObject::kTag; }

inline uword RawHeapObject::baseAddress() {
  uword result = address() - RawHeader::kSize;
  if (header()->hasOverflow()) {
    result -= kPointerSize;
  }
  return result;
}

inline RawHeader RawHeapObject::header() {
  return RawHeader::cast(instanceVariableAt(kHeaderOffset));
}

inline void RawHeapObject::setHeader(RawHeader header) {
  instanceVariableAtPut(kHeaderOffset, header);
}

inline word RawHeapObject::headerOverflow() {
  DCHECK(header()->hasOverflow(), "expected Overflow");
  return RawSmallInt::cast(instanceVariableAt(kHeaderOverflowOffset))->value();
}

inline void RawHeapObject::setHeaderAndOverflow(word count, word hash,
                                                LayoutId id,
                                                ObjectFormat format) {
  if (count > RawHeader::kCountMax) {
    instanceVariableAtPut(kHeaderOverflowOffset, RawSmallInt::fromWord(count));
    count = RawHeader::kCountOverflowFlag;
  }
  setHeader(RawHeader::from(count, hash, id, format));
}

inline RawHeapObject RawHeapObject::fromAddress(uword address) {
  DCHECK((address & kTagMask) == 0, "invalid cast, expected heap address");
  return cast(RawObject{address + kTag});
}

inline word RawHeapObject::headerCountOrOverflow() {
  if (header()->hasOverflow()) {
    return headerOverflow();
  }
  return header()->count();
}

inline word RawHeapObject::size() {
  word count = headerCountOrOverflow();
  word result = headerSize(count);
  switch (header()->format()) {
    case ObjectFormat::kDataArray8:
      result += count;
      break;
    case ObjectFormat::kDataArray16:
      result += count * 2;
      break;
    case ObjectFormat::kDataArray32:
      result += count * 4;
      break;
    case ObjectFormat::kDataArray64:
      result += count * 8;
      break;
    case ObjectFormat::kDataArray128:
      result += count * 16;
      break;
    case ObjectFormat::kObjectArray:
    case ObjectFormat::kDataInstance:
    case ObjectFormat::kObjectInstance:
      result += count * kPointerSize;
      break;
  }
  return Utils::maximum(Utils::roundUp(result, kPointerSize), kMinimumSize);
}

inline word RawHeapObject::headerSize(word count) {
  word result = kPointerSize;
  if (count > RawHeader::kCountMax) {
    result += kPointerSize;
  }
  return result;
}

inline void RawHeapObject::initialize(word size, RawObject value) {
  for (word offset = RawHeapObject::kSize; offset < size;
       offset += kPointerSize) {
    instanceVariableAtPut(offset, value);
  }
}

inline bool RawHeapObject::isRoot() {
  return header()->format() == ObjectFormat::kObjectArray ||
         header()->format() == ObjectFormat::kObjectInstance;
}

inline bool RawHeapObject::isForwarding() {
  return *reinterpret_cast<uword*>(address() + kHeaderOffset) == kIsForwarded;
}

inline RawObject RawHeapObject::forward() {
  // When a heap object is forwarding, its second word is the forwarding
  // address.
  return *reinterpret_cast<RawObject*>(address() + kHeaderOffset +
                                       kPointerSize);
}

inline void RawHeapObject::forwardTo(RawObject object) {
  // Overwrite the header with the forwarding marker.
  *reinterpret_cast<uword*>(address() + kHeaderOffset) = kIsForwarded;
  // Overwrite the second word with the forwarding addressing.
  *reinterpret_cast<RawObject*>(address() + kHeaderOffset + kPointerSize) =
      object;
}

inline RawObject RawHeapObject::instanceVariableAt(word offset) {
  return *reinterpret_cast<RawObject*>(address() + offset);
}

inline void RawHeapObject::instanceVariableAtPut(word offset, RawObject value) {
  *reinterpret_cast<RawObject*>(address() + offset) = value;
}

// RawBaseException

inline RawObject RawBaseException::args() {
  return instanceVariableAt(kArgsOffset);
}

inline void RawBaseException::setArgs(RawObject args) {
  instanceVariableAtPut(kArgsOffset, args);
}

inline RawObject RawBaseException::traceback() {
  return instanceVariableAt(kTracebackOffset);
}

inline void RawBaseException::setTraceback(RawObject traceback) {
  instanceVariableAtPut(kTracebackOffset, traceback);
}

inline RawObject RawBaseException::cause() {
  return instanceVariableAt(kCauseOffset);
}

inline void RawBaseException::setCause(RawObject cause) {
  return instanceVariableAtPut(kCauseOffset, cause);
}

inline RawObject RawBaseException::context() {
  return instanceVariableAt(kContextOffset);
}

inline void RawBaseException::setContext(RawObject context) {
  return instanceVariableAtPut(kContextOffset, context);
}

// RawStopIteration

inline RawObject RawStopIteration::value() {
  return instanceVariableAt(kValueOffset);
}

inline void RawStopIteration::setValue(RawObject value) {
  instanceVariableAtPut(kValueOffset, value);
}

// RawSystemExit

inline RawObject RawSystemExit::code() {
  return instanceVariableAt(kCodeOffset);
}

inline void RawSystemExit::setCode(RawObject code) {
  instanceVariableAtPut(kCodeOffset, code);
}

// RawImportError

inline RawObject RawImportError::msg() {
  return instanceVariableAt(kMsgOffset);
}

inline void RawImportError::setMsg(RawObject msg) {
  instanceVariableAtPut(kMsgOffset, msg);
}

inline RawObject RawImportError::name() {
  return instanceVariableAt(kNameOffset);
}

inline void RawImportError::setName(RawObject name) {
  instanceVariableAtPut(kNameOffset, name);
}

inline RawObject RawImportError::path() {
  return instanceVariableAt(kPathOffset);
}

inline void RawImportError::setPath(RawObject path) {
  instanceVariableAtPut(kPathOffset, path);
}

// RawType

inline RawObject RawType::mro() { return instanceVariableAt(kMroOffset); }

inline void RawType::setMro(RawObject object_array) {
  instanceVariableAtPut(kMroOffset, object_array);
}

inline RawObject RawType::instanceLayout() {
  return instanceVariableAt(kInstanceLayoutOffset);
}

inline void RawType::setInstanceLayout(RawObject layout) {
  instanceVariableAtPut(kInstanceLayoutOffset, layout);
}

inline RawObject RawType::name() { return instanceVariableAt(kNameOffset); }

inline void RawType::setName(RawObject name) {
  instanceVariableAtPut(kNameOffset, name);
}

inline RawObject RawType::flags() { return instanceVariableAt(kFlagsOffset); }

inline void RawType::setFlags(RawObject value) {
  instanceVariableAtPut(kFlagsOffset, value);
}

inline void RawType::setFlag(RawType::Flag bit) {
  word f = RawSmallInt::cast(flags())->value();
  RawObject new_flag = RawSmallInt::fromWord(f | bit);
  instanceVariableAtPut(kFlagsOffset, new_flag);
}

inline bool RawType::hasFlag(RawType::Flag bit) {
  word f = RawSmallInt::cast(flags())->value();
  return (f & bit) != 0;
}

inline RawObject RawType::dict() { return instanceVariableAt(kDictOffset); }

inline void RawType::setDict(RawObject dict) {
  instanceVariableAtPut(kDictOffset, dict);
}

inline RawObject RawType::builtinBaseClass() {
  return instanceVariableAt(kBuiltinBaseClassOffset);
}

inline RawObject RawType::extensionType() {
  return instanceVariableAt(kExtensionTypeOffset);
}

inline void RawType::setExtensionType(RawObject pytype) {
  instanceVariableAtPut(kExtensionTypeOffset, pytype);
}

inline void RawType::setBuiltinBaseClass(RawObject base) {
  instanceVariableAtPut(kBuiltinBaseClassOffset, base);
}

inline bool RawType::isIntrinsicOrExtension() {
  return RawLayout::cast(instanceLayout())->id() <= LayoutId::kLastBuiltinId;
}

// RawArray

inline word RawArray::length() {
  DCHECK(isBytes() || isObjectArray() || isLargeStr(), "invalid array type");
  return headerCountOrOverflow();
}

// RawBytes

inline word RawBytes::allocationSize(word length) {
  DCHECK(length >= 0, "invalid length %ld", length);
  word size = headerSize(length) + length;
  return Utils::maximum(kMinimumSize, Utils::roundUp(size, kPointerSize));
}

inline byte RawBytes::byteAt(word index) {
  DCHECK_INDEX(index, length());
  return *reinterpret_cast<byte*>(address() + index);
}

inline void RawBytes::byteAtPut(word index, byte value) {
  DCHECK_INDEX(index, length());
  *reinterpret_cast<byte*>(address() + index) = value;
}

// RawObjectArray

inline word RawObjectArray::allocationSize(word length) {
  DCHECK(length >= 0, "invalid length %ld", length);
  word size = headerSize(length) + length * kPointerSize;
  return Utils::maximum(kMinimumSize, Utils::roundUp(size, kPointerSize));
}

inline RawObject RawObjectArray::at(word index) {
  DCHECK_INDEX(index, length());
  return instanceVariableAt(index * kPointerSize);
}

inline void RawObjectArray::atPut(word index, RawObject value) {
  DCHECK_INDEX(index, length());
  instanceVariableAtPut(index * kPointerSize, value);
}

inline void RawObjectArray::copyTo(RawObject array) {
  RawObjectArray dst = RawObjectArray::cast(array);
  word len = length();
  DCHECK_BOUND(len, dst->length());
  for (word i = 0; i < len; i++) {
    RawObject elem = at(i);
    dst->atPut(i, elem);
  }
}

inline void RawObjectArray::replaceFromWith(word start, RawObject array) {
  RawObjectArray src = RawObjectArray::cast(array);
  word count = Utils::minimum(this->length() - start, src->length());
  word stop = start + count;
  for (word i = start, j = 0; i < stop; i++, j++) {
    atPut(i, src->at(j));
  }
}

inline bool RawObjectArray::contains(RawObject object) {
  word len = length();
  for (word i = 0; i < len; i++) {
    if (at(i) == object) {
      return true;
    }
  }
  return false;
}

// RawCode

inline word RawCode::argcount() {
  return RawSmallInt::cast(instanceVariableAt(kArgcountOffset))->value();
}

inline void RawCode::setArgcount(word value) {
  instanceVariableAtPut(kArgcountOffset, RawSmallInt::fromWord(value));
}

inline word RawCode::cell2arg() {
  return RawSmallInt::cast(instanceVariableAt(kCell2argOffset))->value();
}

inline word RawCode::totalArgs() {
  uword f = flags();
  word res = argcount() + kwonlyargcount();
  if (f & VARARGS) {
    res++;
  }
  if (f & VARKEYARGS) {
    res++;
  }
  return res;
}

inline void RawCode::setCell2arg(word value) {
  instanceVariableAtPut(kCell2argOffset, RawSmallInt::fromWord(value));
}

inline RawObject RawCode::cellvars() {
  return instanceVariableAt(kCellvarsOffset);
}

inline void RawCode::setCellvars(RawObject value) {
  instanceVariableAtPut(kCellvarsOffset, value);
}

inline word RawCode::numCellvars() {
  RawObject object = cellvars();
  DCHECK(object->isNoneType() || object->isObjectArray(),
         "not an object array");
  if (object->isNoneType()) {
    return 0;
  }
  return RawObjectArray::cast(object)->length();
}

inline RawObject RawCode::code() { return instanceVariableAt(kCodeOffset); }

inline void RawCode::setCode(RawObject value) {
  instanceVariableAtPut(kCodeOffset, value);
}

inline RawObject RawCode::consts() { return instanceVariableAt(kConstsOffset); }

inline void RawCode::setConsts(RawObject value) {
  instanceVariableAtPut(kConstsOffset, value);
}

inline RawObject RawCode::filename() {
  return instanceVariableAt(kFilenameOffset);
}

inline void RawCode::setFilename(RawObject value) {
  instanceVariableAtPut(kFilenameOffset, value);
}

inline word RawCode::firstlineno() {
  return RawSmallInt::cast(instanceVariableAt(kFirstlinenoOffset))->value();
}

inline void RawCode::setFirstlineno(word value) {
  instanceVariableAtPut(kFirstlinenoOffset, RawSmallInt::fromWord(value));
}

inline word RawCode::flags() {
  return RawSmallInt::cast(instanceVariableAt(kFlagsOffset))->value();
}

inline void RawCode::setFlags(word value) {
  if ((kwonlyargcount() == 0) && (value & NOFREE) &&
      !(value & (VARARGS | VARKEYARGS))) {
    // RawSet up shortcut for detecting fast case for calls
    // TODO: move into equivalent of CPython's codeobject.c:PyCode_New()
    value |= SIMPLE_CALL;
  }
  instanceVariableAtPut(kFlagsOffset, RawSmallInt::fromWord(value));
}

inline RawObject RawCode::freevars() {
  return instanceVariableAt(kFreevarsOffset);
}

inline void RawCode::setFreevars(RawObject value) {
  instanceVariableAtPut(kFreevarsOffset, value);
}

inline word RawCode::numFreevars() {
  RawObject object = freevars();
  DCHECK(object->isNoneType() || object->isObjectArray(),
         "not an object array");
  if (object->isNoneType()) {
    return 0;
  }
  return RawObjectArray::cast(object)->length();
}

inline word RawCode::kwonlyargcount() {
  return RawSmallInt::cast(instanceVariableAt(kKwonlyargcountOffset))->value();
}

inline void RawCode::setKwonlyargcount(word value) {
  instanceVariableAtPut(kKwonlyargcountOffset, RawSmallInt::fromWord(value));
}

inline RawObject RawCode::lnotab() { return instanceVariableAt(kLnotabOffset); }

inline void RawCode::setLnotab(RawObject value) {
  instanceVariableAtPut(kLnotabOffset, value);
}

inline RawObject RawCode::name() { return instanceVariableAt(kNameOffset); }

inline void RawCode::setName(RawObject value) {
  instanceVariableAtPut(kNameOffset, value);
}

inline RawObject RawCode::names() { return instanceVariableAt(kNamesOffset); }

inline void RawCode::setNames(RawObject value) {
  instanceVariableAtPut(kNamesOffset, value);
}

inline word RawCode::nlocals() {
  return RawSmallInt::cast(instanceVariableAt(kNlocalsOffset))->value();
}

inline void RawCode::setNlocals(word value) {
  instanceVariableAtPut(kNlocalsOffset, RawSmallInt::fromWord(value));
}

inline word RawCode::totalVars() {
  return nlocals() + numCellvars() + numFreevars();
}

inline word RawCode::stacksize() {
  return RawSmallInt::cast(instanceVariableAt(kStacksizeOffset))->value();
}

inline void RawCode::setStacksize(word value) {
  instanceVariableAtPut(kStacksizeOffset, RawSmallInt::fromWord(value));
}

inline RawObject RawCode::varnames() {
  return instanceVariableAt(kVarnamesOffset);
}

inline void RawCode::setVarnames(RawObject value) {
  instanceVariableAtPut(kVarnamesOffset, value);
}

// RawLargeInt

inline word RawLargeInt::asWord() {
  DCHECK(numDigits() == 1, "RawLargeInt cannot fit in a word");
  return static_cast<word>(digitAt(0));
}

inline void* RawLargeInt::asCPtr() {
  DCHECK(numDigits() == 1, "Large integer cannot fit in a pointer");
  DCHECK(isPositive(), "Cannot cast a negative value to a C pointer");
  return reinterpret_cast<void*>(asWord());
}

template <typename T>
if_signed_t<T, OptInt<T>> RawLargeInt::asInt() {
  static_assert(sizeof(T) <= sizeof(word), "T must not be larger than word");

  if (numDigits() > 1) {
    auto const high_digit = static_cast<word>(digitAt(numDigits() - 1));
    return high_digit < 0 ? OptInt<T>::underflow() : OptInt<T>::overflow();
  }
  if (numDigits() == 1) {
    auto const value = asWord();
    if (value <= std::numeric_limits<T>::max() &&
        value >= std::numeric_limits<T>::min()) {
      return OptInt<T>::valid(value);
    }
  }
  return OptInt<T>::overflow();
}

template <typename T>
if_unsigned_t<T, OptInt<T>> RawLargeInt::asInt() {
  static_assert(sizeof(T) <= sizeof(word), "T must not be larger than word");

  if (isNegative()) return OptInt<T>::underflow();
  if (static_cast<size_t>(bitLength()) > sizeof(T) * kBitsPerByte) {
    return OptInt<T>::overflow();
  }
  // No T accepted by this function needs more than one digit.
  return OptInt<T>::valid(digitAt(0));
}

inline bool RawLargeInt::isNegative() {
  word highest_digit = digitAt(numDigits() - 1);
  return highest_digit < 0;
}

inline bool RawLargeInt::isPositive() {
  word highest_digit = digitAt(numDigits() - 1);
  return highest_digit >= 0;
}

inline word RawLargeInt::digitAt(word index) {
  DCHECK_INDEX(index, numDigits());
  return reinterpret_cast<word*>(address() + kValueOffset)[index];
}

inline void RawLargeInt::digitAtPut(word index, word digit) {
  DCHECK_INDEX(index, numDigits());
  reinterpret_cast<word*>(address() + kValueOffset)[index] = digit;
}

inline word RawLargeInt::numDigits() { return headerCountOrOverflow(); }

inline word RawLargeInt::allocationSize(word num_digits) {
  word size = headerSize(num_digits) + num_digits * kPointerSize;
  return Utils::maximum(kMinimumSize, Utils::roundUp(size, kPointerSize));
}

// RawFloat

inline double RawFloat::value() {
  return *reinterpret_cast<double*>(address() + kValueOffset);
}

inline void RawFloat::initialize(double value) {
  *reinterpret_cast<double*>(address() + kValueOffset) = value;
}

// RawComplex
inline double RawComplex::real() {
  return *reinterpret_cast<double*>(address() + kRealOffset);
}

inline double RawComplex::imag() {
  return *reinterpret_cast<double*>(address() + kImagOffset);
}

inline void RawComplex::initialize(double real, double imag) {
  *reinterpret_cast<double*>(address() + kRealOffset) = real;
  *reinterpret_cast<double*>(address() + kImagOffset) = imag;
}

// RawRange

inline word RawRange::start() {
  return RawSmallInt::cast(instanceVariableAt(kStartOffset))->value();
}

inline void RawRange::setStart(word value) {
  instanceVariableAtPut(kStartOffset, RawSmallInt::fromWord(value));
}

inline word RawRange::stop() {
  return RawSmallInt::cast(instanceVariableAt(kStopOffset))->value();
}

inline void RawRange::setStop(word value) {
  instanceVariableAtPut(kStopOffset, RawSmallInt::fromWord(value));
}

inline word RawRange::step() {
  return RawSmallInt::cast(instanceVariableAt(kStepOffset))->value();
}

inline void RawRange::setStep(word value) {
  instanceVariableAtPut(kStepOffset, RawSmallInt::fromWord(value));
}

// RawListIterator

inline word RawListIterator::index() {
  return RawSmallInt::cast(instanceVariableAt(kIndexOffset))->value();
}

inline void RawListIterator::setIndex(word index) {
  instanceVariableAtPut(kIndexOffset, RawSmallInt::fromWord(index));
}

inline RawObject RawListIterator::list() {
  return instanceVariableAt(kListOffset);
}

inline void RawListIterator::setList(RawObject list) {
  instanceVariableAtPut(kListOffset, list);
}

inline RawObject RawListIterator::next() {
  word idx = index();
  auto underlying = RawList::cast(list());
  if (idx >= underlying->numItems()) {
    return RawError::object();
  }

  RawObject item = underlying->at(idx);
  setIndex(idx + 1);
  return item;
}

// RawProperty

inline RawObject RawProperty::getter() {
  return instanceVariableAt(kGetterOffset);
}

inline void RawProperty::setGetter(RawObject function) {
  instanceVariableAtPut(kGetterOffset, function);
}

inline RawObject RawProperty::setter() {
  return instanceVariableAt(kSetterOffset);
}

inline void RawProperty::setSetter(RawObject function) {
  instanceVariableAtPut(kSetterOffset, function);
}

inline RawObject RawProperty::deleter() {
  return instanceVariableAt(kDeleterOffset);
}

inline void RawProperty::setDeleter(RawObject function) {
  instanceVariableAtPut(kDeleterOffset, function);
}

// RawRangeIterator

inline void RawRangeIterator::setRange(RawObject range) {
  auto r = RawRange::cast(range);
  instanceVariableAtPut(kRangeOffset, r);
  instanceVariableAtPut(kCurValueOffset, RawSmallInt::fromWord(r->start()));
}

inline bool RawRangeIterator::isOutOfRange(word cur, word stop, word step) {
  DCHECK(step != 0,
         "invalid step");  // should have been checked in builtinRange().

  if (step < 0) {
    if (cur <= stop) {
      return true;
    }
  } else if (step > 0) {
    if (cur >= stop) {
      return true;
    }
  }
  return false;
}

inline word RawRangeIterator::pendingLength() {
  RawRange range = RawRange::cast(instanceVariableAt(kRangeOffset));
  word stop = range->stop();
  word step = range->step();
  word current =
      RawSmallInt::cast(instanceVariableAt(kCurValueOffset))->value();
  if (isOutOfRange(current, stop, step)) {
    return 0;
  }
  return std::abs((stop - current) / step);
}

inline RawObject RawRangeIterator::next() {
  auto ret = RawSmallInt::cast(instanceVariableAt(kCurValueOffset));
  auto cur = ret->value();

  auto range = RawRange::cast(instanceVariableAt(kRangeOffset));
  auto stop = range->stop();
  auto step = range->step();

  // TODO: range overflow is unchecked. Since a correct implementation
  // has to support arbitrary precision anyway, there's no point in checking
  // for overflow.
  if (isOutOfRange(cur, stop, step)) {
    // TODO: Use RawStopIteration for control flow.
    return RawError::object();
  }

  instanceVariableAtPut(kCurValueOffset, RawSmallInt::fromWord(cur + step));
  return ret;
}

// RawSlice

inline RawObject RawSlice::start() { return instanceVariableAt(kStartOffset); }

inline void RawSlice::setStart(RawObject value) {
  instanceVariableAtPut(kStartOffset, value);
}

inline RawObject RawSlice::stop() { return instanceVariableAt(kStopOffset); }

inline void RawSlice::setStop(RawObject value) {
  instanceVariableAtPut(kStopOffset, value);
}

inline RawObject RawSlice::step() { return instanceVariableAt(kStepOffset); }

inline void RawSlice::setStep(RawObject value) {
  instanceVariableAtPut(kStepOffset, value);
}

// RawStaticMethod

inline RawObject RawStaticMethod::function() {
  return instanceVariableAt(kFunctionOffset);
}

inline void RawStaticMethod::setFunction(RawObject function) {
  instanceVariableAtPut(kFunctionOffset, function);
}

// RawDict

inline word RawDict::numItems() {
  return RawSmallInt::cast(instanceVariableAt(kNumItemsOffset))->value();
}

inline void RawDict::setNumItems(word num_items) {
  instanceVariableAtPut(kNumItemsOffset, RawSmallInt::fromWord(num_items));
}

inline RawObject RawDict::data() { return instanceVariableAt(kDataOffset); }

inline void RawDict::setData(RawObject data) {
  instanceVariableAtPut(kDataOffset, data);
}

// RawFunction

inline RawObject RawFunction::annotations() {
  return instanceVariableAt(kAnnotationsOffset);
}

inline void RawFunction::setAnnotations(RawObject annotations) {
  return instanceVariableAtPut(kAnnotationsOffset, annotations);
}

inline RawObject RawFunction::closure() {
  return instanceVariableAt(kClosureOffset);
}

inline void RawFunction::setClosure(RawObject closure) {
  return instanceVariableAtPut(kClosureOffset, closure);
}

inline RawObject RawFunction::code() { return instanceVariableAt(kCodeOffset); }

inline void RawFunction::setCode(RawObject code) {
  return instanceVariableAtPut(kCodeOffset, code);
}

inline RawObject RawFunction::defaults() {
  return instanceVariableAt(kDefaultsOffset);
}

inline void RawFunction::setDefaults(RawObject defaults) {
  return instanceVariableAtPut(kDefaultsOffset, defaults);
}

inline bool RawFunction::hasDefaults() { return !defaults()->isNoneType(); }

inline RawObject RawFunction::doc() { return instanceVariableAt(kDocOffset); }

inline void RawFunction::setDoc(RawObject doc) {
  instanceVariableAtPut(kDocOffset, doc);
}

inline RawFunction::Entry RawFunction::entry() {
  RawObject object = instanceVariableAt(kEntryOffset);
  DCHECK(object->isSmallInt(), "entry address must look like a RawSmallInt");
  return bit_cast<RawFunction::Entry>(object);
}

inline void RawFunction::setEntry(RawFunction::Entry entry) {
  auto object = RawSmallInt::fromFunctionPointer(entry);
  instanceVariableAtPut(kEntryOffset, object);
}

inline RawFunction::Entry RawFunction::entryKw() {
  RawObject object = instanceVariableAt(kEntryKwOffset);
  DCHECK(object->isSmallInt(), "entryKw address must look like a RawSmallInt");
  return bit_cast<RawFunction::Entry>(object);
}

inline void RawFunction::setEntryKw(RawFunction::Entry entry_kw) {
  auto object = RawSmallInt::fromFunctionPointer(entry_kw);
  instanceVariableAtPut(kEntryKwOffset, object);
}

RawFunction::Entry RawFunction::entryEx() {
  RawObject object = instanceVariableAt(kEntryExOffset);
  DCHECK(object->isSmallInt(), "entryEx address must look like a RawSmallInt");
  return bit_cast<RawFunction::Entry>(object);
}

void RawFunction::setEntryEx(RawFunction::Entry entry_ex) {
  auto object = RawSmallInt::fromFunctionPointer(entry_ex);
  instanceVariableAtPut(kEntryExOffset, object);
}

inline RawObject RawFunction::globals() {
  return instanceVariableAt(kGlobalsOffset);
}

inline void RawFunction::setGlobals(RawObject globals) {
  return instanceVariableAtPut(kGlobalsOffset, globals);
}

inline RawObject RawFunction::kwDefaults() {
  return instanceVariableAt(kKwDefaultsOffset);
}

inline void RawFunction::setKwDefaults(RawObject kw_defaults) {
  return instanceVariableAtPut(kKwDefaultsOffset, kw_defaults);
}

inline RawObject RawFunction::module() {
  return instanceVariableAt(kModuleOffset);
}

inline void RawFunction::setModule(RawObject module) {
  return instanceVariableAtPut(kModuleOffset, module);
}

inline RawObject RawFunction::name() { return instanceVariableAt(kNameOffset); }

inline void RawFunction::setName(RawObject name) {
  instanceVariableAtPut(kNameOffset, name);
}

inline RawObject RawFunction::qualname() {
  return instanceVariableAt(kQualnameOffset);
}

inline void RawFunction::setQualname(RawObject qualname) {
  instanceVariableAtPut(kQualnameOffset, qualname);
}

inline RawObject RawFunction::fastGlobals() {
  return instanceVariableAt(kFastGlobalsOffset);
}

inline void RawFunction::setFastGlobals(RawObject fast_globals) {
  return instanceVariableAtPut(kFastGlobalsOffset, fast_globals);
}

// RawInstance

inline word RawInstance::allocationSize(word num_attr) {
  DCHECK(num_attr >= 0, "invalid number of attributes %ld", num_attr);
  word size = headerSize(num_attr) + num_attr * kPointerSize;
  return Utils::maximum(kMinimumSize, Utils::roundUp(size, kPointerSize));
}

// RawList

inline RawObject RawList::items() { return instanceVariableAt(kItemsOffset); }

inline void RawList::setItems(RawObject new_items) {
  instanceVariableAtPut(kItemsOffset, new_items);
}

inline word RawList::capacity() {
  return RawObjectArray::cast(items())->length();
}

inline word RawList::numItems() {
  return RawSmallInt::cast(instanceVariableAt(kAllocatedOffset))->value();
}

inline void RawList::setNumItems(word num_items) {
  instanceVariableAtPut(kAllocatedOffset, RawSmallInt::fromWord(num_items));
}

inline void RawList::atPut(word index, RawObject value) {
  DCHECK_INDEX(index, numItems());
  RawObject items = instanceVariableAt(kItemsOffset);
  RawObjectArray::cast(items)->atPut(index, value);
}

inline RawObject RawList::at(word index) {
  DCHECK_INDEX(index, numItems());
  return RawObjectArray::cast(items())->at(index);
}

// RawModule

inline RawObject RawModule::name() { return instanceVariableAt(kNameOffset); }

inline void RawModule::setName(RawObject name) {
  instanceVariableAtPut(kNameOffset, name);
}

inline RawObject RawModule::dict() { return instanceVariableAt(kDictOffset); }

inline void RawModule::setDict(RawObject dict) {
  instanceVariableAtPut(kDictOffset, dict);
}

inline RawObject RawModule::def() { return instanceVariableAt(kDefOffset); }

inline void RawModule::setDef(RawObject dict) {
  instanceVariableAtPut(kDefOffset, dict);
}

// RawStr

inline bool RawStr::equalsCStr(const char* c_str) {
  const char* cp = c_str;
  const word len = length();
  for (word i = 0; i < len; i++, cp++) {
    char ch = *cp;
    if (ch == '\0' || ch != charAt(i)) {
      return false;
    }
  }
  return *cp == '\0';
}

inline byte RawStr::charAt(word index) {
  if (isSmallStr()) {
    return RawSmallStr::cast(*this)->charAt(index);
  }
  DCHECK(isLargeStr(), "unexpected type");
  return RawLargeStr::cast(*this)->charAt(index);
}

inline word RawStr::length() {
  if (isSmallStr()) {
    return RawSmallStr::cast(*this)->length();
  }
  DCHECK(isLargeStr(), "unexpected type");
  return RawLargeStr::cast(*this)->length();
}

inline word RawStr::compare(RawObject string) {
  RawStr that = RawStr::cast(string);
  word length = Utils::minimum(this->length(), that->length());
  for (word i = 0; i < length; i++) {
    word diff = this->charAt(i) - that->charAt(i);
    if (diff != 0) {
      return (diff > 0) ? 1 : -1;
    }
  }
  word diff = this->length() - that->length();
  return (diff > 0) ? 1 : ((diff < 0) ? -1 : 0);
}

inline bool RawStr::equals(RawObject that) {
  if (isSmallStr()) {
    return *this == that;
  }
  DCHECK(isLargeStr(), "unexpected type");
  return RawLargeStr::cast(*this)->equals(that);
}

inline void RawStr::copyTo(byte* dst, word length) {
  if (isSmallStr()) {
    RawSmallStr::cast(*this)->copyTo(dst, length);
    return;
  }
  DCHECK(isLargeStr(), "unexpected type");
  return RawLargeStr::cast(*this)->copyTo(dst, length);
}

inline char* RawStr::toCStr() {
  if (isSmallStr()) {
    return RawSmallStr::cast(*this)->toCStr();
  }
  DCHECK(isLargeStr(), "unexpected type");
  return RawLargeStr::cast(*this)->toCStr();
}

// RawLargeStr

inline word RawLargeStr::allocationSize(word length) {
  DCHECK(length > RawSmallStr::kMaxLength, "length %ld overflows", length);
  word size = headerSize(length) + length;
  return Utils::maximum(kMinimumSize, Utils::roundUp(size, kPointerSize));
}

inline byte RawLargeStr::charAt(word index) {
  DCHECK_INDEX(index, length());
  return *reinterpret_cast<byte*>(address() + index);
}

// RawValueCell

inline RawObject RawValueCell::value() {
  return instanceVariableAt(kValueOffset);
}

inline void RawValueCell::setValue(RawObject object) {
  instanceVariableAtPut(kValueOffset, object);
}

inline bool RawValueCell::isUnbound() { return *this == value(); }

inline void RawValueCell::makeUnbound() { setValue(*this); }

// RawSet

inline word RawSet::numItems() {
  return RawSmallInt::cast(instanceVariableAt(kNumItemsOffset))->value();
}

inline void RawSet::setNumItems(word num_items) {
  instanceVariableAtPut(kNumItemsOffset, RawSmallInt::fromWord(num_items));
}

inline RawObject RawSet::data() { return instanceVariableAt(kDataOffset); }

inline void RawSet::setData(RawObject data) {
  instanceVariableAtPut(kDataOffset, data);
}

// RawBoundMethod

inline RawObject RawBoundMethod::function() {
  return instanceVariableAt(kFunctionOffset);
}

inline void RawBoundMethod::setFunction(RawObject function) {
  instanceVariableAtPut(kFunctionOffset, function);
}

inline RawObject RawBoundMethod::self() {
  return instanceVariableAt(kSelfOffset);
}

inline void RawBoundMethod::setSelf(RawObject self) {
  instanceVariableAtPut(kSelfOffset, self);
}

// RawClassMethod

inline RawObject RawClassMethod::function() {
  return instanceVariableAt(kFunctionOffset);
}

inline void RawClassMethod::setFunction(RawObject function) {
  instanceVariableAtPut(kFunctionOffset, function);
}

// RawWeakRef

inline RawObject RawWeakRef::referent() {
  return instanceVariableAt(kReferentOffset);
}

inline void RawWeakRef::setReferent(RawObject referent) {
  instanceVariableAtPut(kReferentOffset, referent);
}

inline RawObject RawWeakRef::callback() {
  return instanceVariableAt(kCallbackOffset);
}

inline void RawWeakRef::setCallback(RawObject callable) {
  instanceVariableAtPut(kCallbackOffset, callable);
}

inline RawObject RawWeakRef::link() { return instanceVariableAt(kLinkOffset); }

inline void RawWeakRef::setLink(RawObject reference) {
  instanceVariableAtPut(kLinkOffset, reference);
}

// RawLayout

inline LayoutId RawLayout::id() {
  return static_cast<LayoutId>(header()->hashCode());
}

inline void RawLayout::setId(LayoutId id) {
  setHeader(header()->withHashCode(static_cast<word>(id)));
}

inline void RawLayout::setDescribedClass(RawObject type) {
  instanceVariableAtPut(kDescribedClassOffset, type);
}

inline RawObject RawLayout::describedClass() {
  return instanceVariableAt(kDescribedClassOffset);
}

inline void RawLayout::setInObjectAttributes(RawObject attributes) {
  instanceVariableAtPut(kInObjectAttributesOffset, attributes);
}

inline RawObject RawLayout::inObjectAttributes() {
  return instanceVariableAt(kInObjectAttributesOffset);
}

inline void RawLayout::setOverflowAttributes(RawObject attributes) {
  instanceVariableAtPut(kOverflowAttributesOffset, attributes);
}

inline word RawLayout::instanceSize() {
  return RawSmallInt::cast(instanceVariableAt(kInstanceSizeOffset))->value();
}

inline void RawLayout::setInstanceSize(word size) {
  instanceVariableAtPut(kInstanceSizeOffset, RawSmallInt::fromWord(size));
}

inline RawObject RawLayout::overflowAttributes() {
  return instanceVariableAt(kOverflowAttributesOffset);
}

inline void RawLayout::setAdditions(RawObject additions) {
  instanceVariableAtPut(kAdditionsOffset, additions);
}

inline RawObject RawLayout::additions() {
  return instanceVariableAt(kAdditionsOffset);
}

inline void RawLayout::setDeletions(RawObject deletions) {
  instanceVariableAtPut(kDeletionsOffset, deletions);
}

inline RawObject RawLayout::deletions() {
  return instanceVariableAt(kDeletionsOffset);
}

inline word RawLayout::overflowOffset() {
  return RawSmallInt::cast(instanceVariableAt(kOverflowOffsetOffset))->value();
}

inline void RawLayout::setOverflowOffset(word offset) {
  instanceVariableAtPut(kOverflowOffsetOffset, RawSmallInt::fromWord(offset));
}

inline word RawLayout::numInObjectAttributes() {
  return RawSmallInt::cast(instanceVariableAt(kNumInObjectAttributesOffset))
      ->value();
}

inline void RawLayout::setNumInObjectAttributes(word count) {
  instanceVariableAtPut(kNumInObjectAttributesOffset,
                        RawSmallInt::fromWord(count));
  setOverflowOffset(count * kPointerSize);
  setInstanceSize(numInObjectAttributes() + 1);
}

// RawSetIterator

inline RawObject RawSetIterator::set() {
  return instanceVariableAt(kSetOffset);
}

inline void RawSetIterator::setSet(RawObject set) {
  instanceVariableAtPut(kSetOffset, set);
  instanceVariableAtPut(kIndexOffset, RawSmallInt::fromWord(0));
  instanceVariableAtPut(kConsumedCountOffset, RawSmallInt::fromWord(0));
}

inline word RawSetIterator::consumedCount() {
  return RawSmallInt::cast(instanceVariableAt(kConsumedCountOffset))->value();
}

inline void RawSetIterator::setConsumedCount(word consumed) {
  instanceVariableAtPut(kConsumedCountOffset, RawSmallInt::fromWord(consumed));
}

inline word RawSetIterator::index() {
  return RawSmallInt::cast(instanceVariableAt(kIndexOffset))->value();
}

inline void RawSetIterator::setIndex(word index) {
  instanceVariableAtPut(kIndexOffset, RawSmallInt::fromWord(index));
}

inline RawObject RawSetIterator::next() {
  word idx = index();
  RawSet underlying = RawSet::cast(set());
  RawObjectArray data = RawObjectArray::cast(underlying->data());
  word length = data->length();
  // Find the next non empty bucket
  while (idx < length && !RawSet::Bucket::isFilled(data, idx)) {
    idx += RawSet::Bucket::kNumPointers;
  }
  if (idx >= length) {
    return RawError::object();
  }
  setConsumedCount(consumedCount() + 1);
  word new_idx = (idx + RawSet::Bucket::kNumPointers);
  setIndex(new_idx);
  return RawSet::Bucket::key(data, idx);
}

inline word RawSetIterator::pendingLength() {
  RawSet set = RawSet::cast(instanceVariableAt(kSetOffset));
  return set->numItems() - consumedCount();
}

// RawSuper

inline RawObject RawSuper::type() { return instanceVariableAt(kTypeOffset); }

inline void RawSuper::setType(RawObject tp) {
  DCHECK(tp->isType(), "expected type");
  instanceVariableAtPut(kTypeOffset, tp);
}

inline RawObject RawSuper::object() {
  return instanceVariableAt(kObjectOffset);
}

inline void RawSuper::setObject(RawObject obj) {
  instanceVariableAtPut(kObjectOffset, obj);
}

inline RawObject RawSuper::objectType() {
  return instanceVariableAt(kObjectTypeOffset);
}

inline void RawSuper::setObjectType(RawObject tp) {
  DCHECK(tp->isType(), "expected type");
  instanceVariableAtPut(kObjectTypeOffset, tp);
}

// RawTupleIterator

inline RawObject RawTupleIterator::tuple() {
  return instanceVariableAt(kTupleOffset);
}

inline void RawTupleIterator::setTuple(RawObject tuple) {
  instanceVariableAtPut(kTupleOffset, tuple);
  instanceVariableAtPut(kIndexOffset, RawSmallInt::fromWord(0));
}

inline word RawTupleIterator::index() {
  return RawSmallInt::cast(instanceVariableAt(kIndexOffset))->value();
}

inline void RawTupleIterator::setIndex(word index) {
  instanceVariableAtPut(kIndexOffset, RawSmallInt::fromWord(index));
}

inline RawObject RawTupleIterator::next() {
  word idx = index();
  RawObjectArray underlying = RawObjectArray::cast(tuple());
  if (idx >= underlying->length()) {
    return RawError::object();
  }

  RawObject item = underlying->at(idx);
  setIndex(idx + 1);
  return item;
}

// RawGeneratorBase

inline RawObject RawGeneratorBase::heapFrame() {
  return instanceVariableAt(kFrameOffset);
}

inline void RawGeneratorBase::setHeapFrame(RawObject obj) {
  instanceVariableAtPut(kFrameOffset, obj);
}

}  // namespace python

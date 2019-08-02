#include "under-builtins-module.h"

#include <cerrno>

#include <unistd.h>

#include "bytearray-builtins.h"
#include "bytes-builtins.h"
#include "dict-builtins.h"
#include "exception-builtins.h"
#include "frozen-modules.h"
#include "int-builtins.h"
#include "list-builtins.h"
#include "object-builtins.h"
#include "range-builtins.h"
#include "str-builtins.h"
#include "tuple-builtins.h"
#include "type-builtins.h"

namespace python {

static bool isPass(const Code& code) {
  HandleScope scope;
  Bytes bytes(&scope, code.code());
  // const_loaded is the index into the consts array that is returned
  word const_loaded = bytes.byteAt(1);
  return bytes.length() == 4 && bytes.byteAt(0) == LOAD_CONST &&
         Tuple::cast(code.consts()).at(const_loaded).isNoneType() &&
         bytes.byteAt(2) == RETURN_VALUE && bytes.byteAt(3) == 0;
}

void copyFunctionEntries(Thread* thread, const Function& base,
                         const Function& patch) {
  HandleScope scope(thread);
  Str method_name(&scope, base.name());
  Code patch_code(&scope, patch.code());
  Code base_code(&scope, base.code());
  CHECK(isPass(patch_code),
        "Redefinition of native code method '%s' in managed code",
        method_name.toCStr());
  CHECK(!base_code.code().isNoneType(),
        "Useless declaration of native code method %s in managed code",
        method_name.toCStr());
  patch_code.setCode(base_code.code());
  patch_code.setLnotab(Bytes::empty());
  patch.setEntry(base.entry());
  patch.setEntryKw(base.entryKw());
  patch.setEntryEx(base.entryEx());
  patch.setIsInterpreted(false);
  patch.setIntrinsicId(base.intrinsicId());
}

static RawObject raiseRequiresFromCaller(Thread* thread, Frame* frame,
                                         word nargs, SymbolId expected_type) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Function function(&scope, frame->previousFrame()->function());
  Str function_name(&scope, function.name());
  Object obj(&scope, args.get(0));
  return thread->raiseWithFmt(LayoutId::kTypeError,
                              "'%S' requires a '%Y' object but received a '%T'",
                              &function_name, expected_type, &obj);
}

const BuiltinMethod UnderBuiltinsModule::kBuiltinMethods[] = {
    {SymbolId::kUnderAddress, underAddress},
    {SymbolId::kUnderBoundMethod, underBoundMethod},
    {SymbolId::kUnderByteArrayCheck, underByteArrayCheck},
    {SymbolId::kUnderByteArrayClear, underByteArrayClear},
    {SymbolId::kUnderByteArrayGuard, underByteArrayGuard},
    {SymbolId::kUnderByteArrayJoin, underByteArrayJoin},
    {SymbolId::kUnderByteArrayLen, underByteArrayLen},
    {SymbolId::kUnderByteArraySetitem, underByteArraySetItem},
    {SymbolId::kUnderBytesCheck, underBytesCheck},
    {SymbolId::kUnderBytesFromInts, underBytesFromInts},
    {SymbolId::kUnderBytesGetitem, underBytesGetItem},
    {SymbolId::kUnderBytesGetslice, underBytesGetSlice},
    {SymbolId::kUnderBytesGuard, underBytesGuard},
    {SymbolId::kUnderBytesJoin, underBytesJoin},
    {SymbolId::kUnderBytesLen, underBytesLen},
    {SymbolId::kUnderBytesMaketrans, underBytesMaketrans},
    {SymbolId::kUnderBytesRepeat, underBytesRepeat},
    {SymbolId::kUnderByteslikeEndsWith, underByteslikeEndsWith},
    {SymbolId::kUnderByteslikeFindByteslike, underByteslikeFindByteslike},
    {SymbolId::kUnderByteslikeFindInt, underByteslikeFindInt},
    {SymbolId::kUnderClassMethod, underClassMethod},
    {SymbolId::kUnderClassMethodIsAbstract, underClassMethodIsAbstract},
    {SymbolId::kUnderComplexImag, underComplexImag},
    {SymbolId::kUnderComplexReal, underComplexReal},
    {SymbolId::kUnderDictBucketInsert, underDictBucketInsert},
    {SymbolId::kUnderDictBucketKey, underDictBucketKey},
    {SymbolId::kUnderDictBucketUpdate, underDictBucketUpdate},
    {SymbolId::kUnderDictBucketValue, underDictBucketValue},
    {SymbolId::kUnderDictCheck, underDictCheck},
    {SymbolId::kUnderDictGuard, underDictGuard},
    {SymbolId::kUnderDictLen, underDictLen},
    {SymbolId::kUnderDictLookup, underDictLookup},
    {SymbolId::kUnderDictLookupNext, underDictLookupNext},
    {SymbolId::kUnderDictUpdateMapping, underDictUpdateMapping},
    {SymbolId::kUnderDivmod, underDivmod},
    {SymbolId::kUnderFloatCheck, underFloatCheck},
    {SymbolId::kUnderFloatGuard, underFloatGuard},
    {SymbolId::kUnderFrozenSetCheck, underFrozenSetCheck},
    {SymbolId::kUnderGetMemberByte, underGetMemberByte},
    {SymbolId::kUnderGetMemberChar, underGetMemberChar},
    {SymbolId::kUnderGetMemberDouble, underGetMemberDouble},
    {SymbolId::kUnderGetMemberFloat, underGetMemberFloat},
    {SymbolId::kUnderGetMemberInt, underGetMemberInt},
    {SymbolId::kUnderGetMemberLong, underGetMemberLong},
    {SymbolId::kUnderGetMemberPyObject, underGetMemberPyObject},
    {SymbolId::kUnderGetMemberShort, underGetMemberShort},
    {SymbolId::kUnderGetMemberString, underGetMemberString},
    {SymbolId::kUnderGetMemberUByte, underGetMemberUByte},
    {SymbolId::kUnderGetMemberUInt, underGetMemberUInt},
    {SymbolId::kUnderGetMemberULong, underGetMemberULong},
    {SymbolId::kUnderGetMemberUShort, underGetMemberUShort},
    {SymbolId::kUnderInstanceGetattr, underInstanceGetattr},
    {SymbolId::kUnderInstanceSetattr, underInstanceSetattr},
    {SymbolId::kUnderIntCheck, underIntCheck},
    {SymbolId::kUnderIntCheckExact, underIntCheckExact},
    {SymbolId::kUnderIntFromBytes, underIntFromBytes},
    {SymbolId::kUnderIntGuard, underIntGuard},
    {SymbolId::kUnderIntNewFromByteArray, underIntNewFromByteArray},
    {SymbolId::kUnderIntNewFromBytes, underIntNewFromBytes},
    {SymbolId::kUnderIntNewFromInt, underIntNewFromInt},
    {SymbolId::kUnderIntNewFromStr, underIntNewFromStr},
    {SymbolId::kUnderListCheck, underListCheck},
    {SymbolId::kUnderListCheckExact, underListCheckExact},
    {SymbolId::kUnderListDelitem, underListDelItem},
    {SymbolId::kUnderListDelslice, underListDelSlice},
    {SymbolId::kUnderListExtend, underListExtend},
    {SymbolId::kUnderListGetitem, underListGetItem},
    {SymbolId::kUnderListGetslice, underListGetSlice},
    {SymbolId::kUnderListGuard, underListGuard},
    {SymbolId::kUnderListLen, underListLen},
    {SymbolId::kUnderListSort, underListSort},
    {SymbolId::kUnderObjectTypeGetattr, underObjectTypeGetAttr},
    {SymbolId::kUnderObjectTypeHasattr, underObjectTypeHasattr},
    {SymbolId::kUnderOsRead, underOsRead},
    {SymbolId::kUnderOsWrite, underOsWrite},
    {SymbolId::kUnderProperty, underProperty},
    {SymbolId::kUnderPropertyIsAbstract, underPropertyIsAbstract},
    {SymbolId::kUnderPyObjectOffset, underPyObjectOffset},
    {SymbolId::kUnderRangeCheck, underRangeCheck},
    {SymbolId::kUnderRangeGuard, underRangeGuard},
    {SymbolId::kUnderRangeLen, underRangeLen},
    {SymbolId::kUnderReprEnter, underReprEnter},
    {SymbolId::kUnderReprLeave, underReprLeave},
    {SymbolId::kUnderSeqIndex, underSeqIndex},
    {SymbolId::kUnderSeqIterable, underSeqIterable},
    {SymbolId::kUnderSeqSetIndex, underSeqSetIndex},
    {SymbolId::kUnderSeqSetIterable, underSeqSetIterable},
    {SymbolId::kUnderSetCheck, underSetCheck},
    {SymbolId::kUnderSetGuard, underSetGuard},
    {SymbolId::kUnderSetLen, underSetLen},
    {SymbolId::kUnderSetMemberDouble, underSetMemberDouble},
    {SymbolId::kUnderSetMemberFloat, underSetMemberFloat},
    {SymbolId::kUnderSetMemberIntegral, underSetMemberIntegral},
    {SymbolId::kUnderSetMemberPyObject, underSetMemberPyObject},
    {SymbolId::kUnderSliceCheck, underSliceCheck},
    {SymbolId::kUnderSliceGuard, underSliceGuard},
    {SymbolId::kUnderSliceStart, underSliceStart},
    {SymbolId::kUnderSliceStep, underSliceStep},
    {SymbolId::kUnderSliceStop, underSliceStop},
    {SymbolId::kUnderStaticMethodIsAbstract, underStaticMethodIsAbstract},
    {SymbolId::kUnderStrArrayIadd, underStrArrayIadd},
    {SymbolId::kUnderStrCheck, underStrCheck},
    {SymbolId::kUnderStrCheckExact, underStrCheckExact},
    {SymbolId::kUnderStrCount, underStrCount},
    {SymbolId::kUnderStrGuard, underStrGuard},
    {SymbolId::kUnderStrJoin, underStrJoin},
    {SymbolId::kUnderStrEscapeNonAscii, underStrEscapeNonAscii},
    {SymbolId::kUnderStrFind, underStrFind},
    {SymbolId::kUnderStrFromStr, underStrFromStr},
    {SymbolId::kUnderStrLen, underStrLen},
    {SymbolId::kUnderStrReplace, underStrReplace},
    {SymbolId::kUnderStrRFind, underStrRFind},
    {SymbolId::kUnderStrSplitlines, underStrSplitlines},
    {SymbolId::kUnderTupleCheck, underTupleCheck},
    {SymbolId::kUnderTupleCheckExact, underTupleCheckExact},
    {SymbolId::kUnderTupleGuard, underTupleGuard},
    {SymbolId::kUnderTupleLen, underTupleLen},
    {SymbolId::kUnderTupleNew, underTupleNew},
    {SymbolId::kUnderType, underType},
    {SymbolId::kUnderTypeAbstractMethodsDel, underTypeAbstractMethodsDel},
    {SymbolId::kUnderTypeAbstractMethodsGet, underTypeAbstractMethodsGet},
    {SymbolId::kUnderTypeAbstractMethodsSet, underTypeAbstractMethodsSet},
    {SymbolId::kUnderTypeBasesDel, underTypeBasesDel},
    {SymbolId::kUnderTypeBasesGet, underTypeBasesGet},
    {SymbolId::kUnderTypeBasesSet, underTypeBasesSet},
    {SymbolId::kUnderTypeCheck, underTypeCheck},
    {SymbolId::kUnderTypeCheckExact, underTypeCheckExact},
    {SymbolId::kUnderTypeDictKeys, underTypeDictKeys},
    {SymbolId::kUnderTypeGuard, underTypeGuard},
    {SymbolId::kUnderTypeIsSubclass, underTypeIsSubclass},
    {SymbolId::kUnderUnimplemented, underUnimplemented},
    {SymbolId::kSentinelId, nullptr},
};

const BuiltinType UnderBuiltinsModule::kBuiltinTypes[] = {
    {SymbolId::kUnderTraceback, LayoutId::kTraceback},
    {SymbolId::kSentinelId, LayoutId::kSentinelId},
};

const char* const UnderBuiltinsModule::kFrozenData = kUnderBuiltinsModuleData;

const SymbolId UnderBuiltinsModule::kIntrinsicIds[] = {
    SymbolId::kUnderByteArrayCheck,  SymbolId::kUnderByteArrayGuard,
    SymbolId::kUnderByteArrayLen,    SymbolId::kUnderBytesCheck,
    SymbolId::kUnderBytesGuard,      SymbolId::kUnderBytesLen,
    SymbolId::kUnderDictCheck,       SymbolId::kUnderDictGuard,
    SymbolId::kUnderDictLen,         SymbolId::kUnderFloatCheck,
    SymbolId::kUnderFloatGuard,      SymbolId::kUnderFrozenSetCheck,
    SymbolId::kUnderIntCheck,        SymbolId::kUnderIntCheckExact,
    SymbolId::kUnderIntGuard,        SymbolId::kUnderListCheck,
    SymbolId::kUnderListCheckExact,  SymbolId::kUnderListGetitem,
    SymbolId::kUnderListGuard,       SymbolId::kUnderListLen,
    SymbolId::kUnderRangeCheck,      SymbolId::kUnderRangeGuard,
    SymbolId::kUnderSetCheck,        SymbolId::kUnderSetGuard,
    SymbolId::kUnderSetLen,          SymbolId::kUnderSliceCheck,
    SymbolId::kUnderSliceGuard,      SymbolId::kUnderStrCheck,
    SymbolId::kUnderStrCheckExact,   SymbolId::kUnderStrGuard,
    SymbolId::kUnderStrLen,          SymbolId::kUnderTupleCheck,
    SymbolId::kUnderTupleCheckExact, SymbolId::kUnderTupleGuard,
    SymbolId::kUnderTupleLen,        SymbolId::kUnderType,
    SymbolId::kUnderTypeCheck,       SymbolId::kUnderTypeCheckExact,
    SymbolId::kUnderTypeGuard,       SymbolId::kSentinelId,
};

RawObject UnderBuiltinsModule::underAddress(Thread* thread, Frame* frame,
                                            word nargs) {
  Arguments args(frame, nargs);
  return thread->runtime()->newInt(args.get(0).raw());
}

RawObject UnderBuiltinsModule::underBoundMethod(Thread* thread, Frame* frame,
                                                word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object function(&scope, args.get(0));
  Object owner(&scope, args.get(1));
  return thread->runtime()->newBoundMethod(function, owner);
}

RawObject UnderBuiltinsModule::underByteArrayClear(Thread* thread, Frame* frame,
                                                   word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  ByteArray self(&scope, args.get(0));
  self.downsize(0);
  return NoneType::object();
}

RawObject UnderBuiltinsModule::underByteArrayCheck(Thread* thread, Frame* frame,
                                                   word nargs) {
  Arguments args(frame, nargs);
  return Bool::fromBool(thread->runtime()->isInstanceOfByteArray(args.get(0)));
}

RawObject UnderBuiltinsModule::underByteArrayGuard(Thread* thread, Frame* frame,
                                                   word nargs) {
  Arguments args(frame, nargs);
  if (thread->runtime()->isInstanceOfByteArray(args.get(0))) {
    return NoneType::object();
  }
  return raiseRequiresFromCaller(thread, frame, nargs, SymbolId::kByteArray);
}

RawObject UnderBuiltinsModule::underByteArraySetItem(Thread* thread,
                                                     Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  ByteArray self(&scope, args.get(0));
  Object key_obj(&scope, args.get(1));
  Int key(&scope, intUnderlying(thread, key_obj));
  Object value_obj(&scope, args.get(2));
  Int value(&scope, intUnderlying(thread, value_obj));
  word index = key.asWordSaturated();
  if (!SmallInt::isValid(index)) {
    return thread->raiseWithFmt(LayoutId::kIndexError,
                                "cannot fit '%T' into an index-sized integer",
                                &key_obj);
  }
  word length = self.numItems();
  if (index < 0) {
    index += length;
  }
  if (index < 0 || index >= length) {
    return thread->raiseWithFmt(LayoutId::kIndexError, "index out of range");
  }
  word val = value.asWordSaturated();
  if (val < 0 || val > kMaxByte) {
    return thread->raiseWithFmt(LayoutId::kValueError,
                                "byte must be in range(0, 256)");
  }
  self.byteAtPut(index, val);
  return NoneType::object();
}

RawObject UnderBuiltinsModule::underBytesCheck(Thread* thread, Frame* frame,
                                               word nargs) {
  Arguments args(frame, nargs);
  return Bool::fromBool(thread->runtime()->isInstanceOfBytes(args.get(0)));
}

RawObject UnderBuiltinsModule::underBytesGuard(Thread* thread, Frame* frame,
                                               word nargs) {
  Arguments args(frame, nargs);
  if (thread->runtime()->isInstanceOfBytes(args.get(0))) {
    return NoneType::object();
  }
  return raiseRequiresFromCaller(thread, frame, nargs, SymbolId::kBytes);
}

RawObject UnderBuiltinsModule::underByteArrayJoin(Thread* thread, Frame* frame,
                                                  word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  ByteArray sep(&scope, args.get(0));
  Bytes sep_bytes(&scope, sep.bytes());
  Object iterable(&scope, args.get(1));
  Object joined(&scope, NoneType::object());
  Runtime* runtime = thread->runtime();
  if (iterable.isList()) {
    List list(&scope, *iterable);
    Tuple src(&scope, list.items());
    joined = runtime->bytesJoin(thread, sep_bytes, sep.numItems(), src,
                                list.numItems());
  } else if (iterable.isTuple()) {
    Tuple src(&scope, *iterable);
    joined = runtime->bytesJoin(thread, sep_bytes, sep.numItems(), src,
                                src.length());
  }
  // Check for error or slow path
  if (!joined.isMutableBytes()) return *joined;
  MutableBytes joined_bytes(&scope, *joined);
  ByteArray result(&scope, runtime->newByteArray());
  result.setBytes(*joined_bytes);
  result.setNumItems(joined_bytes.length());
  return *result;
}

RawObject UnderBuiltinsModule::underByteArrayLen(Thread* thread, Frame* frame,
                                                 word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  ByteArray self(&scope, args.get(0));
  return SmallInt::fromWord(self.numItems());
}

RawObject UnderBuiltinsModule::underBytesFromInts(Thread* thread, Frame* frame,
                                                  word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object src(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  // TODO(T38246066): buffers other than bytes, bytearray
  if (runtime->isInstanceOfBytes(*src)) {
    return *src;
  }
  if (runtime->isInstanceOfByteArray(*src)) {
    ByteArray source(&scope, *src);
    return byteArrayAsBytes(thread, runtime, source);
  }
  if (src.isList()) {
    List source(&scope, *src);
    Tuple items(&scope, source.items());
    return runtime->bytesFromTuple(thread, items, source.numItems());
  }
  if (src.isTuple()) {
    Tuple source(&scope, *src);
    return runtime->bytesFromTuple(thread, source, source.length());
  }
  if (runtime->isInstanceOfStr(*src)) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "cannot convert '%T' object to bytes", &src);
  }
  // Slow path: iterate over source in Python, collect into list, and call again
  return NoneType::object();
}

RawObject UnderBuiltinsModule::underBytesGetItem(Thread* thread, Frame* frame,
                                                 word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  Bytes self(&scope, bytesUnderlying(thread, self_obj));
  Object key_obj(&scope, args.get(1));
  Int key(&scope, intUnderlying(thread, key_obj));
  word index = key.asWordSaturated();
  if (!SmallInt::isValid(index)) {
    return thread->raiseWithFmt(LayoutId::kIndexError,
                                "cannot fit '%T' into an index-sized integer",
                                &key_obj);
  }
  word length = self.length();
  if (index < 0) {
    index += length;
  }
  if (index < 0 || index >= length) {
    return thread->raiseWithFmt(LayoutId::kIndexError, "index out of range");
  }
  return SmallInt::fromWord(self.byteAt(index));
}

RawObject UnderBuiltinsModule::underBytesGetSlice(Thread* thread, Frame* frame,
                                                  word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  Bytes self(&scope, bytesUnderlying(thread, self_obj));
  Object obj(&scope, args.get(1));
  Int start(&scope, intUnderlying(thread, obj));
  obj = args.get(2);
  Int stop(&scope, intUnderlying(thread, obj));
  obj = args.get(3);
  Int step(&scope, intUnderlying(thread, obj));
  return thread->runtime()->bytesSlice(thread, self, start.asWord(),
                                       stop.asWord(), step.asWord());
}

RawObject UnderBuiltinsModule::underBytesJoin(Thread* thread, Frame* frame,
                                              word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  Bytes self(&scope, bytesUnderlying(thread, self_obj));
  Object iterable(&scope, args.get(1));
  if (iterable.isList()) {
    List list(&scope, *iterable);
    Tuple src(&scope, list.items());
    return thread->runtime()->bytesJoin(thread, self, self.length(), src,
                                        list.numItems());
  }
  if (iterable.isTuple()) {
    Tuple src(&scope, *iterable);
    return thread->runtime()->bytesJoin(thread, self, self.length(), src,
                                        src.length());
  }
  // Slow path: collect items into list in Python and call again
  return NoneType::object();
}

RawObject UnderBuiltinsModule::underBytesLen(Thread* thread, Frame* frame,
                                             word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  Bytes self(&scope, bytesUnderlying(thread, self_obj));
  return SmallInt::fromWord(self.length());
}

RawObject UnderBuiltinsModule::underBytesMaketrans(Thread* thread, Frame* frame,
                                                   word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object from_obj(&scope, args.get(0));
  Object to_obj(&scope, args.get(1));
  word length;
  Runtime* runtime = thread->runtime();
  if (runtime->isInstanceOfBytes(*from_obj)) {
    Bytes bytes(&scope, bytesUnderlying(thread, from_obj));
    length = bytes.length();
    from_obj = *bytes;
  } else if (runtime->isInstanceOfByteArray(*from_obj)) {
    ByteArray array(&scope, *from_obj);
    length = array.numItems();
    from_obj = array.bytes();
  } else {
    UNIMPLEMENTED("bytes-like other than bytes or bytearray");
  }
  if (runtime->isInstanceOfBytes(*to_obj)) {
    Bytes bytes(&scope, bytesUnderlying(thread, to_obj));
    DCHECK(bytes.length() == length, "lengths should already be the same");
    to_obj = *bytes;
  } else if (runtime->isInstanceOfByteArray(*to_obj)) {
    ByteArray array(&scope, *to_obj);
    DCHECK(array.numItems() == length, "lengths should already be the same");
    to_obj = array.bytes();
  } else {
    UNIMPLEMENTED("bytes-like other than bytes or bytearray");
  }
  Bytes from(&scope, *from_obj);
  Bytes to(&scope, *to_obj);
  byte table[BytesBuiltins::kTranslationTableLength];
  for (word i = 0; i < BytesBuiltins::kTranslationTableLength; i++) {
    table[i] = i;
  }
  for (word i = 0; i < length; i++) {
    table[from.byteAt(i)] = to.byteAt(i);
  }
  return runtime->newBytesWithAll(table);
}

RawObject UnderBuiltinsModule::underBytesRepeat(Thread* thread, Frame* frame,
                                                word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  Bytes self(&scope, bytesUnderlying(thread, self_obj));
  Object count_obj(&scope, args.get(1));
  Int count_int(&scope, intUnderlying(thread, count_obj));
  word count = count_int.asWordSaturated();
  if (!SmallInt::isValid(count)) {
    return thread->raiseWithFmt(LayoutId::kOverflowError,
                                "cannot fit '%T' into an index-sized integer",
                                &count_obj);
  }
  // NOTE: unlike __mul__, we raise a value error for negative count
  if (count < 0) {
    return thread->raiseWithFmt(LayoutId::kValueError, "negative count");
  }
  return thread->runtime()->bytesRepeat(thread, self, self.length(), count);
}

RawObject UnderBuiltinsModule::underByteslikeEndsWith(Thread* thread,
                                                      Frame* frame,
                                                      word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Runtime* runtime = thread->runtime();
  Object self_obj(&scope, args.get(0));
  word self_len;
  if (runtime->isInstanceOfBytes(*self_obj)) {
    Bytes self(&scope, bytesUnderlying(thread, self_obj));
    self_obj = *self;
    self_len = self.length();
  } else if (runtime->isInstanceOfByteArray(*self_obj)) {
    ByteArray self(&scope, *self_obj);
    self_obj = self.bytes();
    self_len = self.numItems();
  } else {
    UNREACHABLE("self has an unexpected type");
  }
  DCHECK(self_obj.isBytes(),
         "bytes-like object not resolved to underlying bytes");
  Object suffix_obj(&scope, args.get(1));
  word suffix_len;
  if (runtime->isInstanceOfBytes(*suffix_obj)) {
    Bytes suffix(&scope, bytesUnderlying(thread, suffix_obj));
    suffix_obj = *suffix;
    suffix_len = suffix.length();
  } else if (runtime->isInstanceOfByteArray(*suffix_obj)) {
    ByteArray suffix(&scope, *suffix_obj);
    suffix_obj = suffix.bytes();
    suffix_len = suffix.numItems();
  } else {
    // TODO(T38246066): support buffer protocol
    return thread->raiseWithFmt(
        LayoutId::kTypeError,
        "endswith first arg must be bytes or a tuple of bytes, not %T",
        &suffix_obj);
  }
  Bytes self(&scope, *self_obj);
  Bytes suffix(&scope, *suffix_obj);
  Object start_obj(&scope, args.get(2));
  Object end_obj(&scope, args.get(3));
  Int start(&scope, start_obj.isUnbound() ? SmallInt::fromWord(0)
                                          : intUnderlying(thread, start_obj));
  Int end(&scope, end_obj.isUnbound() ? SmallInt::fromWord(self_len)
                                      : intUnderlying(thread, end_obj));
  return runtime->bytesEndsWith(self, self_len, suffix, suffix_len,
                                start.asWordSaturated(), end.asWordSaturated());
}

RawObject UnderBuiltinsModule::underByteslikeFindByteslike(Thread* thread,
                                                           Frame* frame,
                                                           word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Runtime* runtime = thread->runtime();
  Object self_obj(&scope, args.get(0));
  word haystack_len;
  if (runtime->isInstanceOfBytes(*self_obj)) {
    Bytes self(&scope, bytesUnderlying(thread, self_obj));
    self_obj = *self;
    haystack_len = self.length();
  } else if (runtime->isInstanceOfByteArray(*self_obj)) {
    ByteArray self(&scope, *self_obj);
    self_obj = self.bytes();
    haystack_len = self.numItems();
  } else {
    UNIMPLEMENTED("bytes-like other than bytes, bytearray");
  }
  Object sub_obj(&scope, args.get(1));
  word needle_len;
  if (runtime->isInstanceOfBytes(*sub_obj)) {
    Bytes sub(&scope, bytesUnderlying(thread, sub_obj));
    sub_obj = *sub;
    needle_len = sub.length();
  } else if (runtime->isInstanceOfByteArray(*sub_obj)) {
    ByteArray sub(&scope, *sub_obj);
    sub_obj = sub.bytes();
    needle_len = sub.numItems();
  } else {
    UNIMPLEMENTED("bytes-like other than bytes, bytearray");
  }
  Bytes haystack(&scope, *self_obj);
  Bytes needle(&scope, *sub_obj);
  Object start_obj(&scope, args.get(2));
  Object stop_obj(&scope, args.get(3));
  Int start(&scope, intUnderlying(thread, start_obj));
  Int end(&scope, intUnderlying(thread, stop_obj));
  return bytesFind(haystack, haystack_len, needle, needle_len,
                   start.asWordSaturated(), end.asWordSaturated());
}

RawObject UnderBuiltinsModule::underByteslikeFindInt(Thread* thread,
                                                     Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Runtime* runtime = thread->runtime();
  Object sub_obj(&scope, args.get(1));
  Int sub_int(&scope, intUnderlying(thread, sub_obj));
  word sub = sub_int.asWordSaturated();
  if (sub < 0 || sub > kMaxByte) {
    return thread->raiseWithFmt(LayoutId::kValueError,
                                "byte must be in range(0, 256)");
  }
  Bytes needle(&scope, runtime->newBytes(1, sub));
  Object self_obj(&scope, args.get(0));
  Object start_obj(&scope, args.get(2));
  Object stop_obj(&scope, args.get(3));
  Int start(&scope, intUnderlying(thread, start_obj));
  Int end(&scope, intUnderlying(thread, stop_obj));
  if (runtime->isInstanceOfBytes(*self_obj)) {
    Bytes haystack(&scope, bytesUnderlying(thread, self_obj));
    return bytesFind(haystack, haystack.length(), needle, needle.length(),
                     start.asWordSaturated(), end.asWordSaturated());
  }
  if (runtime->isInstanceOfByteArray(*self_obj)) {
    ByteArray self(&scope, *self_obj);
    Bytes haystack(&scope, self.bytes());
    return bytesFind(haystack, self.numItems(), needle, needle.length(),
                     start.asWordSaturated(), end.asWordSaturated());
  }
  UNIMPLEMENTED("bytes-like other than bytes, bytearray");
}

RawObject UnderBuiltinsModule::underClassMethod(Thread* thread, Frame* frame,
                                                word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  ClassMethod result(&scope, thread->runtime()->newClassMethod());
  result.setFunction(args.get(0));
  return *result;
}

static RawObject isAbstract(Thread* thread, const Object& obj) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  // TODO(T47800709): make this lookup more efficient
  Object abstract(&scope, runtime->attributeAtId(
                              thread, obj, SymbolId::kDunderIsAbstractMethod));
  if (abstract.isError()) {
    Object given(&scope, thread->pendingExceptionType());
    Object exc(&scope, runtime->typeAt(LayoutId::kAttributeError));
    if (givenExceptionMatches(thread, given, exc)) {
      thread->clearPendingException();
      return Bool::falseObj();
    }
    return *abstract;
  }
  return Interpreter::isTrue(thread, *abstract);
}

RawObject UnderBuiltinsModule::underClassMethodIsAbstract(Thread* thread,
                                                          Frame* frame,
                                                          word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  ClassMethod self(&scope, args.get(0));
  Object func(&scope, self.function());
  return isAbstract(thread, func);
}

RawObject UnderBuiltinsModule::underComplexImag(Thread* thread, Frame* frame,
                                                word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfComplex(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kComplex);
  }
  Complex self(&scope, *self_obj);
  return runtime->newFloat(self.imag());
}

RawObject UnderBuiltinsModule::underComplexReal(Thread* thread, Frame* frame,
                                                word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfComplex(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kComplex);
  }
  Complex self(&scope, *self_obj);
  return runtime->newFloat(self.real());
}

// TODO(T46009010): Move this method body into the dictionary API
RawObject UnderBuiltinsModule::underDictBucketInsert(Thread* thread,
                                                     Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Dict dict(&scope, args.get(0));
  Tuple data(&scope, dict.data());
  word index = ~Int::cast(args.get(1)).asWord();
  Object key(&scope, args.get(2));
  Object key_hash(&scope, args.get(3));
  key_hash = SmallInt::fromWordTruncated(
      Int::cast(intUnderlying(thread, key_hash)).digitAt(0));
  Object value(&scope, args.get(4));
  bool has_empty_slot = Dict::Bucket::isEmpty(*data, index);
  Dict::Bucket::set(*data, index, *key_hash, *key, *value);
  dict.setNumItems(dict.numItems() + 1);
  if (has_empty_slot) {
    dict.decrementNumUsableItems();
    thread->runtime()->dictEnsureCapacity(thread, dict);
  }
  return NoneType::object();
}

RawObject UnderBuiltinsModule::underDictBucketKey(Thread* thread, Frame* frame,
                                                  word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Dict dict(&scope, args.get(0));
  Tuple data(&scope, dict.data());
  word index = Int::cast(args.get(1)).asWord();
  return Dict::Bucket::key(*data, index);
}

RawObject UnderBuiltinsModule::underDictBucketValue(Thread* thread,
                                                    Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Dict dict(&scope, args.get(0));
  Tuple data(&scope, dict.data());
  word index = Int::cast(args.get(1)).asWord();
  return Dict::Bucket::value(*data, index);
}

RawObject UnderBuiltinsModule::underDictBucketUpdate(Thread* thread,
                                                     Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Dict dict(&scope, args.get(0));
  Tuple data(&scope, dict.data());
  word index = Int::cast(args.get(1)).asWord();
  Object key(&scope, args.get(2));
  Object key_hash(&scope, args.get(3));
  key_hash = SmallInt::fromWordTruncated(
      Int::cast(intUnderlying(thread, key_hash)).digitAt(0));
  Object value(&scope, args.get(4));
  Dict::Bucket::set(*data, index, *key_hash, *key, *value);
  return NoneType::object();
}

RawObject UnderBuiltinsModule::underDictCheck(Thread* thread, Frame* frame,
                                              word nargs) {
  Arguments args(frame, nargs);
  return Bool::fromBool(thread->runtime()->isInstanceOfDict(args.get(0)));
}

RawObject UnderBuiltinsModule::underDictGuard(Thread* thread, Frame* frame,
                                              word nargs) {
  Arguments args(frame, nargs);
  if (thread->runtime()->isInstanceOfDict(args.get(0))) {
    return NoneType::object();
  }
  return raiseRequiresFromCaller(thread, frame, nargs, SymbolId::kDict);
}

RawObject UnderBuiltinsModule::underDictLen(Thread* thread, Frame* frame,
                                            word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Dict self(&scope, args.get(0));
  return SmallInt::fromWord(self.numItems());
}

RawObject UnderBuiltinsModule::underDictLookup(Thread* thread, Frame* frame,
                                               word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object dict_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfDict(*dict_obj)) {
    return thread->raiseWithFmt(
        LayoutId::kTypeError,
        "_dict_lookup expected a 'dict' self but got '%T'", &dict_obj);
  }
  Dict dict(&scope, *dict_obj);
  Object key(&scope, args.get(1));
  Object key_hash(&scope, args.get(2));
  if (!runtime->isInstanceOfInt(*key_hash)) {
    return thread->raiseRequiresType(key_hash, SymbolId::kInt);
  }
  key_hash = intUnderlying(thread, key_hash);
  key_hash = SmallInt::fromWordTruncated(Int::cast(*key_hash).digitAt(0));
  if (dict.capacity() == 0) {
    dict.setData(runtime->newTuple(Runtime::kInitialDictCapacity *
                                   Dict::Bucket::kNumPointers));
    dict.resetNumUsableItems();
  }
  Tuple data(&scope, dict.data());
  word bucket_mask = Dict::Bucket::bucketMask(data.length());
  uword perturb = static_cast<uword>(RawSmallInt::cast(*key_hash).value());
  word index = Dict::Bucket::reduceIndex(data.length(), perturb);
  // Track the first place where an item could be inserted. This might be
  // the index zero. Therefore, all negative insertion indexes will be
  // offset by one to distinguish the zero index.
  uword insert_idx = 0;
  for (;;) {
    if (Dict::Bucket::isEmpty(*data, index)) {
      if (insert_idx == 0) insert_idx = ~index;
      return SmallInt::fromWord(insert_idx);
    }
    if (Dict::Bucket::isTombstone(*data, index)) {
      if (insert_idx == 0) insert_idx = ~index;
    } else {
      if (key.raw() == Dict::Bucket::key(*data, index).raw()) {
        return SmallInt::fromWord(index);
      }
      if (SmallInt::cast(*key_hash).value() ==
          SmallInt::cast(Dict::Bucket::hash(*data, index)).value()) {
        return SmallInt::fromWord(index);
      }
    }
    index = Dict::Bucket::nextBucket(index / Dict::Bucket::kNumPointers,
                                     bucket_mask, &perturb) *
            Dict::Bucket::kNumPointers;
  }
}

RawObject UnderBuiltinsModule::underDictLookupNext(Thread* thread, Frame* frame,
                                                   word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Dict dict(&scope, args.get(0));
  Tuple data(&scope, dict.data());
  word index = Int::cast(args.get(1)).asWord();
  Object key(&scope, args.get(2));
  Object key_hash(&scope, args.get(3));
  key_hash = SmallInt::fromWordTruncated(
      Int::cast(intUnderlying(thread, key_hash)).digitAt(0));
  uword perturb;
  if (args.get(4).isUnbound()) {
    perturb = static_cast<uword>(RawSmallInt::cast(*key_hash).value());
  } else {
    perturb = Int::cast(args.get(4)).asWord();
  }
  word bucket_mask = Dict::Bucket::bucketMask(data.length());
  Tuple result(&scope, thread->runtime()->newTuple(2));
  word insert_idx = 0;
  for (;;) {
    index = Dict::Bucket::nextBucket(index / Dict::Bucket::kNumPointers,
                                     bucket_mask, &perturb) *
            Dict::Bucket::kNumPointers;
    if (Dict::Bucket::isEmpty(*data, index)) {
      if (insert_idx == 0) insert_idx = ~index;
      result.atPut(0, SmallInt::fromWord(insert_idx));
      result.atPut(1, SmallInt::fromWord(perturb));
      return *result;
    }
    if (Dict::Bucket::isTombstone(*data, index)) {
      if (insert_idx == 0) insert_idx = ~index;
      continue;
    }
    if (key.raw() == Dict::Bucket::key(*data, index).raw()) {
      result.atPut(0, SmallInt::fromWord(index));
      result.atPut(1, SmallInt::fromWord(perturb));
      return *result;
    }
    if (SmallInt::cast(*key_hash).value() ==
        SmallInt::cast(Dict::Bucket::hash(*data, index)).value()) {
      result.atPut(0, SmallInt::fromWord(index));
      result.atPut(1, SmallInt::fromWord(perturb));
      return *result;
    }
  }
}

RawObject UnderBuiltinsModule::underDictUpdateMapping(Thread* thread,
                                                      Frame* frame,
                                                      word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  Object other(&scope, args.get(1));
  Runtime* runtime = thread->runtime();
  DCHECK(runtime->isInstanceOfDict(*self_obj), "self must be instance of dict");
  Type other_type(&scope, runtime->typeOf(*other));
  DCHECK(!typeLookupSymbolInMro(thread, other_type, SymbolId::kKeys).isError(),
         "other must have 'keys' method");
  Dict self(&scope, *self_obj);
  return dictMergeOverride(thread, self, other);
}

RawObject UnderBuiltinsModule::underDivmod(Thread* thread, Frame* frame,
                                           word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object number(&scope, args.get(0));
  Object divisor(&scope, args.get(1));
  return Interpreter::binaryOperation(
      thread, frame, Interpreter::BinaryOp::DIVMOD, number, divisor);
}

RawObject UnderBuiltinsModule::underFloatCheck(Thread* thread, Frame* frame,
                                               word nargs) {
  Arguments args(frame, nargs);
  return Bool::fromBool(thread->runtime()->isInstanceOfFloat(args.get(0)));
}

RawObject UnderBuiltinsModule::underFloatGuard(Thread* thread, Frame* frame,
                                               word nargs) {
  Arguments args(frame, nargs);
  if (thread->runtime()->isInstanceOfFloat(args.get(0))) {
    return NoneType::object();
  }
  return raiseRequiresFromCaller(thread, frame, nargs, SymbolId::kFloat);
}

RawObject UnderBuiltinsModule::underFrozenSetCheck(Thread* thread, Frame* frame,
                                                   word nargs) {
  Arguments args(frame, nargs);
  return Bool::fromBool(thread->runtime()->isInstanceOfFrozenSet(args.get(0)));
}

RawObject UnderBuiltinsModule::underGetMemberByte(Thread* thread, Frame* frame,
                                                  word nargs) {
  Arguments args(frame, nargs);
  auto addr = Int::cast(args.get(0)).asCPtr();
  char value = 0;
  std::memcpy(&value, reinterpret_cast<void*>(addr), 1);
  return thread->runtime()->newInt(value);
}

RawObject UnderBuiltinsModule::underGetMemberChar(Thread*, Frame* frame,
                                                  word nargs) {
  Arguments args(frame, nargs);
  auto addr = Int::cast(args.get(0)).asCPtr();
  return SmallStr::fromCodePoint(*reinterpret_cast<byte*>(addr));
}

RawObject UnderBuiltinsModule::underGetMemberDouble(Thread* thread,
                                                    Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  auto addr = Int::cast(args.get(0)).asCPtr();
  double value = 0.0;
  std::memcpy(&value, reinterpret_cast<void*>(addr), sizeof(value));
  return thread->runtime()->newFloat(value);
}

RawObject UnderBuiltinsModule::underGetMemberFloat(Thread* thread, Frame* frame,
                                                   word nargs) {
  Arguments args(frame, nargs);
  auto addr = Int::cast(args.get(0)).asCPtr();
  float value = 0.0;
  std::memcpy(&value, reinterpret_cast<void*>(addr), sizeof(value));
  return thread->runtime()->newFloat(value);
}

RawObject UnderBuiltinsModule::underGetMemberInt(Thread* thread, Frame* frame,
                                                 word nargs) {
  Arguments args(frame, nargs);
  auto addr = Int::cast(args.get(0)).asCPtr();
  int value = 0;
  std::memcpy(&value, reinterpret_cast<void*>(addr), sizeof(value));
  return thread->runtime()->newInt(value);
}

RawObject UnderBuiltinsModule::underGetMemberLong(Thread* thread, Frame* frame,
                                                  word nargs) {
  Arguments args(frame, nargs);
  auto addr = Int::cast(args.get(0)).asCPtr();
  long value = 0;
  std::memcpy(&value, reinterpret_cast<void*>(addr), sizeof(value));
  return thread->runtime()->newInt(value);
}

RawObject UnderBuiltinsModule::underGetMemberPyObject(Thread* thread,
                                                      Frame* frame,
                                                      word nargs) {
  Arguments args(frame, nargs);
  auto addr = Int::cast(args.get(0)).asCPtr();
  auto pyobject = reinterpret_cast<PyObject**>(addr);
  if (*pyobject == nullptr) {
    if (args.get(1).isNoneType()) return NoneType::object();
    HandleScope scope(thread);
    Str name(&scope, args.get(1));
    return thread->raiseWithFmt(LayoutId::kAttributeError,
                                "Object attribute '%S' is nullptr", &name);
  }
  return ApiHandle::fromPyObject(*pyobject)->asObject();
}

RawObject UnderBuiltinsModule::underGetMemberShort(Thread* thread, Frame* frame,
                                                   word nargs) {
  Arguments args(frame, nargs);
  auto addr = Int::cast(args.get(0)).asCPtr();
  short value = 0;
  std::memcpy(&value, reinterpret_cast<void*>(addr), sizeof(value));
  return thread->runtime()->newInt(value);
}

RawObject UnderBuiltinsModule::underGetMemberString(Thread* thread,
                                                    Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  auto addr = Int::cast(args.get(0)).asCPtr();
  return thread->runtime()->newStrFromCStr(*reinterpret_cast<char**>(addr));
}

RawObject UnderBuiltinsModule::underGetMemberUByte(Thread* thread, Frame* frame,
                                                   word nargs) {
  Arguments args(frame, nargs);
  auto addr = Int::cast(args.get(0)).asCPtr();
  unsigned char value = 0;
  std::memcpy(&value, reinterpret_cast<void*>(addr), sizeof(value));
  return thread->runtime()->newIntFromUnsigned(value);
}

RawObject UnderBuiltinsModule::underGetMemberUInt(Thread* thread, Frame* frame,
                                                  word nargs) {
  Arguments args(frame, nargs);
  auto addr = Int::cast(args.get(0)).asCPtr();
  unsigned int value = 0;
  std::memcpy(&value, reinterpret_cast<void*>(addr), sizeof(value));
  return thread->runtime()->newIntFromUnsigned(value);
}

RawObject UnderBuiltinsModule::underGetMemberULong(Thread* thread, Frame* frame,
                                                   word nargs) {
  Arguments args(frame, nargs);
  auto addr = Int::cast(args.get(0)).asCPtr();
  unsigned long value = 0;
  std::memcpy(&value, reinterpret_cast<void*>(addr), sizeof(value));
  return thread->runtime()->newIntFromUnsigned(value);
}

RawObject UnderBuiltinsModule::underGetMemberUShort(Thread* thread,
                                                    Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  auto addr = Int::cast(args.get(0)).asCPtr();
  unsigned short value = 0;
  std::memcpy(&value, reinterpret_cast<void*>(addr), sizeof(value));
  return thread->runtime()->newIntFromUnsigned(value);
}

RawObject UnderBuiltinsModule::underInstanceGetattr(Thread* thread,
                                                    Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  HeapObject instance(&scope, args.get(0));
  Object name(&scope, args.get(1));
  return instanceGetAttribute(thread, instance, name);
}

RawObject UnderBuiltinsModule::underInstanceSetattr(Thread* thread,
                                                    Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  HeapObject instance(&scope, args.get(0));
  Object name(&scope, args.get(1));
  Object value(&scope, args.get(2));
  return instanceSetAttr(thread, instance, name, value);
}

RawObject UnderBuiltinsModule::underIntCheck(Thread* thread, Frame* frame,
                                             word nargs) {
  Arguments args(frame, nargs);
  return Bool::fromBool(thread->runtime()->isInstanceOfInt(args.get(0)));
}

RawObject UnderBuiltinsModule::underIntCheckExact(Thread*, Frame* frame,
                                                  word nargs) {
  Arguments args(frame, nargs);
  RawObject arg = args.get(0);
  return Bool::fromBool(arg.isSmallInt() || arg.isLargeInt());
}

static RawObject intOrUserSubclass(Thread* thread, const Type& type,
                                   const Object& value) {
  DCHECK(value.isSmallInt() || value.isLargeInt(),
         "builtin value should have type int");
  DCHECK(type.builtinBase() == LayoutId::kInt, "type must subclass int");
  if (type.isBuiltin()) return *value;
  HandleScope scope(thread);
  Layout layout(&scope, type.instanceLayout());
  UserIntBase instance(&scope, thread->runtime()->newInstance(layout));
  instance.setValue(*value);
  return *instance;
}

RawObject UnderBuiltinsModule::underIntFromBytes(Thread* thread, Frame* frame,
                                                 word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Runtime* runtime = thread->runtime();

  Type type(&scope, args.get(0));
  Bytes bytes(&scope, args.get(1));
  Bool byteorder_big(&scope, args.get(2));
  endian endianness = byteorder_big.value() ? endian::big : endian::little;
  Bool signed_arg(&scope, args.get(3));
  bool is_signed = signed_arg == Bool::trueObj();
  Int value(&scope, runtime->bytesToInt(thread, bytes, endianness, is_signed));
  return intOrUserSubclass(thread, type, value);
}

RawObject UnderBuiltinsModule::underIntGuard(Thread* thread, Frame* frame,
                                             word nargs) {
  Arguments args(frame, nargs);
  if (thread->runtime()->isInstanceOfInt(args.get(0))) {
    return NoneType::object();
  }
  return raiseRequiresFromCaller(thread, frame, nargs, SymbolId::kInt);
}

static RawObject intFromBytes(Thread* /* t */, const Bytes& bytes, word length,
                              word base) {
  DCHECK_BOUND(length, bytes.length());
  DCHECK(base == 0 || (base >= 2 && base <= 36), "invalid base");
  if (length == 0) {
    return Error::error();
  }
  unique_c_ptr<char[]> str(reinterpret_cast<char*>(std::malloc(length + 1)));
  bytes.copyTo(reinterpret_cast<byte*>(str.get()), length);
  str[length] = '\0';
  char* end;
  errno = 0;
  const word result = std::strtoll(str.get(), &end, base);
  const int saved_errno = errno;
  if (end != str.get() + length || saved_errno == EINVAL) {
    return Error::error();
  }
  if (SmallInt::isValid(result) && saved_errno != ERANGE) {
    return SmallInt::fromWord(result);
  }
  UNIMPLEMENTED("LargeInt from bytes-like");
}

RawObject UnderBuiltinsModule::underIntNewFromByteArray(Thread* thread,
                                                        Frame* frame,
                                                        word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Type type(&scope, args.get(0));
  ByteArray array(&scope, args.get(1));
  Bytes bytes(&scope, array.bytes());
  Object base_obj(&scope, args.get(2));
  Int base_int(&scope, intUnderlying(thread, base_obj));
  DCHECK(base_int.numDigits() == 1, "invalid base");
  word base = base_int.asWord();
  Object result(&scope, intFromBytes(thread, bytes, array.numItems(), base));
  if (result.isError()) {
    Runtime* runtime = thread->runtime();
    Bytes truncated(&scope, byteArrayAsBytes(thread, runtime, array));
    Str repr(&scope, bytesReprSmartQuotes(thread, truncated));
    return thread->raiseWithFmt(LayoutId::kValueError,
                                "invalid literal for int() with base %w: %S",
                                base, &repr);
  }
  return intOrUserSubclass(thread, type, result);
}

RawObject UnderBuiltinsModule::underIntNewFromBytes(Thread* thread,
                                                    Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Type type(&scope, args.get(0));
  Object bytes_obj(&scope, args.get(1));
  Bytes bytes(&scope, bytesUnderlying(thread, bytes_obj));
  Object base_obj(&scope, args.get(2));
  Int base_int(&scope, intUnderlying(thread, base_obj));
  DCHECK(base_int.numDigits() == 1, "invalid base");
  word base = base_int.asWord();
  Object result(&scope, intFromBytes(thread, bytes, bytes.length(), base));
  if (result.isError()) {
    Str repr(&scope, bytesReprSmartQuotes(thread, bytes));
    return thread->raiseWithFmt(LayoutId::kValueError,
                                "invalid literal for int() with base %w: %S",
                                base, &repr);
  }
  return intOrUserSubclass(thread, type, result);
}

RawObject UnderBuiltinsModule::underIntNewFromInt(Thread* thread, Frame* frame,
                                                  word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Type type(&scope, args.get(0));
  Object value(&scope, args.get(1));
  if (value.isBool()) {
    value = convertBoolToInt(*value);
  } else if (!value.isSmallInt() && !value.isLargeInt()) {
    value = intUnderlying(thread, value);
  }
  return intOrUserSubclass(thread, type, value);
}

static word digitValue(byte digit, word base) {
  if ('0' <= digit && digit < '0' + base) return digit - '0';
  // Bases 2-10 are limited to numerals, but all greater bases can use letters
  // too.
  if (base <= 10) return -1;
  if ('a' <= digit && digit < 'a' + base) return digit - 'a' + 10;
  if ('A' <= digit && digit < 'A' + base) return digit - 'A' + 10;
  return -1;
}

static word inferBase(const Str& str, word start) {
  if (str.charAt(start) == '0' && start + 1 < str.length()) {
    switch (str.charAt(start + 1)) {
      case 'x':
      case 'X':
        return 16;
      case 'o':
      case 'O':
        return 8;
      case 'b':
      case 'B':
        return 2;
    }
  }
  return 10;
}

static RawObject intFromStr(Thread* thread, const Str& str, word base) {
  DCHECK(base == 0 || (base >= 2 && base <= 36), "invalid base");
  // CPython allows leading whitespace in the integer literal
  word start = strFindFirstNonWhitespace(str);
  if (str.length() - start == 0) {
    return Error::error();
  }
  word sign = 1;
  if (str.charAt(start) == '-') {
    sign = -1;
    start += 1;
  } else if (str.charAt(start) == '+') {
    start += 1;
  }
  if (str.length() - start == 0) {
    // Just the sign
    return Error::error();
  }
  if (str.length() - start == 1) {
    // Single digit, potentially with +/-
    word result = digitValue(str.charAt(start), base == 0 ? 10 : base);
    if (result == -1) return Error::error();
    return SmallInt::fromWord(sign * result);
  }
  // Decimal literals start at the index 0 (no prefix).
  // Octal literals (0oFOO), hex literals (0xFOO), and binary literals (0bFOO)
  // start at index 2.
  word inferred_base = inferBase(str, start);
  if (base == 0) {
    base = inferred_base;
  }
  if (base == 2 || base == 8 || base == 16) {
    if (base == inferred_base) {
      // This handles integer literals with a base prefix, e.g.
      // * int("0b1", 0) => 1, where the base is inferred from the prefix
      // * int("0b1", 2) => 1, where the prefix matches the provided base
      //
      // If the prefix does not match the provided base, then we treat it as
      // part as part of the number, e.g.
      // * int("0b1", 10) => ValueError
      // * int("0b1", 16) => 177
      start += 2;
    }
    if (str.length() - start == 0) {
      // Just the prefix: 0x, 0b, 0o, etc
      return Error::error();
    }
  }
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Int result(&scope, SmallInt::fromWord(0));
  Int digit(&scope, SmallInt::fromWord(0));
  Int base_obj(&scope, SmallInt::fromWord(base));
  for (word i = start; i < str.length(); i++) {
    byte digit_char = str.charAt(i);
    if (digit_char == '_') {
      // No leading underscores unless the number has a prefix
      if (i == start && inferred_base == 10) return Error::error();
      // No trailing underscores
      if (i + 1 == str.length()) return Error::error();
      digit_char = str.charAt(++i);
    }
    word digit_val = digitValue(digit_char, base);
    if (digit_val == -1) return Error::error();
    digit = Int::cast(SmallInt::fromWord(digit_val));
    result = runtime->intMultiply(thread, result, base_obj);
    result = runtime->intAdd(thread, result, digit);
  }
  if (sign < 0) {
    result = runtime->intNegate(thread, result);
  }
  return *result;
}

RawObject UnderBuiltinsModule::underIntNewFromStr(Thread* thread, Frame* frame,
                                                  word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Type type(&scope, args.get(0));
  Str str(&scope, args.get(1));
  Object base_obj(&scope, args.get(2));
  Int base_int(&scope, intUnderlying(thread, base_obj));
  DCHECK(base_int.numDigits() == 1, "invalid base");
  word base = base_int.asWord();
  Object result(&scope, intFromStr(thread, str, base));
  if (result.isError()) {
    Str repr(&scope, thread->invokeMethod1(str, SymbolId::kDunderRepr));
    return thread->raiseWithFmt(LayoutId::kValueError,
                                "invalid literal for int() with base %w: %S",
                                base == 0 ? 10 : base, &repr);
  }
  return intOrUserSubclass(thread, type, result);
}

RawObject UnderBuiltinsModule::underListCheck(Thread* thread, Frame* frame,
                                              word nargs) {
  Arguments args(frame, nargs);
  return Bool::fromBool(thread->runtime()->isInstanceOfList(args.get(0)));
}

RawObject UnderBuiltinsModule::underListCheckExact(Thread*, Frame* frame,
                                                   word nargs) {
  Arguments args(frame, nargs);
  return Bool::fromBool(args.get(0).isList());
}

RawObject UnderBuiltinsModule::underListDelItem(Thread* thread, Frame* frame,
                                                word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  List self(&scope, args.get(0));
  word length = self.numItems();
  Object index_obj(&scope, args.get(1));
  Int index_int(&scope, intUnderlying(thread, index_obj));
  word idx = index_int.asWordSaturated();
  if (idx < 0) {
    idx += length;
  }
  if (idx < 0 || idx >= length) {
    return thread->raiseWithFmt(LayoutId::kIndexError,
                                "list assignment index out of range");
  }
  listPop(thread, self, idx);
  return NoneType::object();
}

RawObject UnderBuiltinsModule::underListDelSlice(Thread* thread, Frame* frame,
                                                 word nargs) {
  // This function deletes elements that are specified by a slice by copying.
  // It compacts to the left elements in the slice range and then copies
  // elements after the slice into the free area.  The list element count is
  // decremented and elements in the unused part of the list are overwritten
  // with None.
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  List list(&scope, args.get(0));

  Object start_obj(&scope, args.get(1));
  Int start_int(&scope, intUnderlying(thread, start_obj));
  word start = start_int.asWord();

  Object stop_obj(&scope, args.get(2));
  Int stop_int(&scope, intUnderlying(thread, stop_obj));
  word stop = stop_int.asWord();

  Object step_obj(&scope, args.get(3));
  Int step_int(&scope, intUnderlying(thread, step_obj));
  // Lossy truncation of step to a word is expected.
  word step = step_int.asWordSaturated();

  word slice_length = Slice::length(start, stop, step);
  DCHECK(slice_length >= 0, "slice length should be positive");
  if (slice_length == 0) {
    // Nothing to delete
    return NoneType::object();
  }
  if (slice_length == list.numItems()) {
    // Delete all the items
    list.clearFrom(0);
    return NoneType::object();
  }
  if (step < 0) {
    // Adjust step to make iterating easier
    start = start + step * (slice_length - 1);
    step = -step;
  }
  DCHECK(start >= 0, "start should be positive");
  DCHECK(start < list.numItems(), "start should be in bounds");
  DCHECK(step <= list.numItems() || slice_length == 1,
         "Step should be in bounds or only one element should be sliced");
  // Sliding compaction of elements out of the slice to the left
  // Invariant: At each iteration of the loop, `fast` is the index of an
  // element addressed by the slice.
  // Invariant: At each iteration of the inner loop, `slow` is the index of a
  // location to where we are relocating a slice addressed element. It is *not*
  // addressed by the slice.
  word fast = start;
  for (word i = 1; i < slice_length; i++) {
    DCHECK_INDEX(fast, list.numItems());
    word slow = fast + 1;
    fast += step;
    for (; slow < fast; slow++) {
      list.atPut(slow - i, list.at(slow));
    }
  }
  // Copy elements into the space where the deleted elements were
  for (word i = fast + 1; i < list.numItems(); i++) {
    list.atPut(i - slice_length, list.at(i));
  }
  word new_length = list.numItems() - slice_length;
  DCHECK(new_length >= 0, "new_length must be positive");
  // Untrack all deleted elements
  list.clearFrom(new_length);
  return NoneType::object();
}

RawObject UnderBuiltinsModule::underListExtend(Thread* thread, Frame* frame,
                                               word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  List list(&scope, args.get(0));
  Object value(&scope, args.get(1));
  return listExtend(thread, list, value);
}

RawObject UnderBuiltinsModule::underListGetItem(Thread* thread, Frame* frame,
                                                word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  List self(&scope, args.get(0));
  Object key_obj(&scope, args.get(1));
  Int key(&scope, intUnderlying(thread, key_obj));
  word index = key.asWordSaturated();
  if (!SmallInt::isValid(index)) {
    return thread->raiseWithFmt(LayoutId::kIndexError,
                                "cannot fit '%T' into an index-sized integer",
                                &key_obj);
  }
  word length = self.numItems();
  if (index < 0) {
    index += length;
  }
  if (index < 0 || index >= length) {
    return thread->raiseWithFmt(LayoutId::kIndexError,
                                "list index out of range");
  }
  return self.at(index);
}

RawObject UnderBuiltinsModule::underListGetSlice(Thread* thread, Frame* frame,
                                                 word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  List self(&scope, args.get(0));
  Object obj(&scope, args.get(1));
  Int start(&scope, intUnderlying(thread, obj));
  obj = args.get(2);
  Int stop(&scope, intUnderlying(thread, obj));
  obj = args.get(3);
  Int step(&scope, intUnderlying(thread, obj));
  return listSlice(thread, self, start.asWord(), stop.asWord(), step.asWord());
}

RawObject UnderBuiltinsModule::underListGuard(Thread* thread, Frame* frame,
                                              word nargs) {
  Arguments args(frame, nargs);
  if (thread->runtime()->isInstanceOfList(args.get(0))) {
    return NoneType::object();
  }
  return raiseRequiresFromCaller(thread, frame, nargs, SymbolId::kList);
}

RawObject UnderBuiltinsModule::underListLen(Thread* thread, Frame* frame,
                                            word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  List self(&scope, args.get(0));
  return SmallInt::fromWord(self.numItems());
}

RawObject UnderBuiltinsModule::underListSort(Thread* thread, Frame* frame,
                                             word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  CHECK(thread->runtime()->isInstanceOfList(args.get(0)),
        "Unsupported argument type for 'ls'");
  List list(&scope, args.get(0));
  return listSort(thread, list);
}

RawObject UnderBuiltinsModule::underObjectTypeGetAttr(Thread* thread,
                                                      Frame* frame,
                                                      word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object instance(&scope, args.get(0));
  Type type(&scope, thread->runtime()->typeOf(*instance));
  Str name(&scope, args.get(1));
  Object attr(&scope, typeLookupNameInMro(thread, type, name));
  if (attr.isErrorNotFound()) {
    return Unbound::object();
  }
  return resolveDescriptorGet(thread, attr, instance, type);
}

RawObject UnderBuiltinsModule::underObjectTypeHasattr(Thread* thread,
                                                      Frame* frame,
                                                      word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Type type(&scope, thread->runtime()->typeOf(args.get(0)));
  Str name(&scope, args.get(1));
  Object result(&scope, typeLookupNameInMro(thread, type, name));
  return Bool::fromBool(!result.isErrorNotFound());
}

RawObject UnderBuiltinsModule::underOsRead(Thread* thread, Frame* frame,
                                           word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object fd_obj(&scope, args.get(0));
  CHECK(fd_obj.isSmallInt(), "fd must be small int");
  Object count_obj(&scope, args.get(1));
  CHECK(count_obj.isSmallInt(), "count must be small int");
  CHECK(!Int::cast(*count_obj).isNegative(), "count must be non-negative");
  size_t count = SmallInt::cast(*count_obj).value();
  ssize_t result;
  std::unique_ptr<byte[]> buffer(new byte[count]{0});
  {
    int fd = SmallInt::cast(*fd_obj).value();
    do {
      errno = 0;
      result = ::read(fd, buffer.get(), count);
    } while (result == -1 && errno == EINTR);
  }
  if (result < 0) {
    DCHECK(errno != EINTR, "this should have been handled in the loop");
    return thread->raiseOSErrorFromErrno(errno);
  }
  return thread->runtime()->newBytesWithAll(View<byte>(buffer.get(), result));
}

RawObject UnderBuiltinsModule::underOsWrite(Thread* thread, Frame* frame,
                                            word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object fd_obj(&scope, args.get(0));
  CHECK(fd_obj.isSmallInt(), "fd must be small int");
  Bytes bytes_buf(&scope, args.get(1));
  size_t count = bytes_buf.length();
  std::unique_ptr<byte[]> buffer(new byte[count]);
  bytes_buf.copyTo(buffer.get(), count);
  ssize_t result;
  {
    int fd = SmallInt::cast(*fd_obj).value();
    do {
      result = ::write(fd, buffer.get(), count);
    } while (result == -1 && errno == EINTR);
  }
  if (result == -1) {
    DCHECK(errno != EINTR, "this should have been handled in the loop");
    return thread->raiseOSErrorFromErrno(errno);
  }
  return SmallInt::fromWord(result);
}

RawObject UnderBuiltinsModule::underPatch(Thread* thread, Frame* frame,
                                          word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);

  Object patch_fn_obj(&scope, args.get(0));
  if (!patch_fn_obj.isFunction()) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "_patch expects function argument");
  }
  Function patch_fn(&scope, *patch_fn_obj);
  Str fn_name(&scope, patch_fn.name());
  Runtime* runtime = thread->runtime();
  Object module_name(&scope, patch_fn.module());
  Module module(&scope, runtime->findModule(module_name));
  Object base_fn_obj(&scope, runtime->moduleAt(module, fn_name));
  if (!base_fn_obj.isFunction()) {
    if (base_fn_obj.isErrorNotFound()) {
      return thread->raiseWithFmt(LayoutId::kAttributeError,
                                  "function %S not found in module %S",
                                  &fn_name, &module_name);
    }
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "_patch can only patch functions");
  }
  Function base_fn(&scope, *base_fn_obj);
  copyFunctionEntries(thread, base_fn, patch_fn);
  return *patch_fn;
}

RawObject UnderBuiltinsModule::underProperty(Thread* thread, Frame* frame,
                                             word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object getter(&scope, args.get(0));
  Object setter(&scope, args.get(1));
  Object deleter(&scope, args.get(2));
  // TODO(T42363565) Do something with the doc argument.
  return thread->runtime()->newProperty(getter, setter, deleter);
}

RawObject UnderBuiltinsModule::underPropertyIsAbstract(Thread* thread,
                                                       Frame* frame,
                                                       word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Property self(&scope, args.get(0));
  Object getter(&scope, self.getter());
  Object abstract(&scope, isAbstract(thread, getter));
  if (abstract != Bool::falseObj()) {
    return *abstract;
  }
  Object setter(&scope, self.setter());
  if ((abstract = isAbstract(thread, setter)) != Bool::falseObj()) {
    return *abstract;
  }
  Object deleter(&scope, self.deleter());
  return isAbstract(thread, deleter);
}

RawObject UnderBuiltinsModule::underPyObjectOffset(Thread* thread, Frame* frame,
                                                   word nargs) {
  // TODO(eelizondo): Remove the HandleScope
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object instance_obj(&scope, args.get(0));
  Int instance(&scope, ApiHandle::getExtensionPtrAttr(thread, instance_obj));
  auto addr = reinterpret_cast<uword>(instance.asCPtr());
  addr += RawInt::cast(args.get(1)).asWord();
  return thread->runtime()->newIntFromCPtr(bit_cast<void*>(addr));
}

RawObject UnderBuiltinsModule::underRangeCheck(Thread*, Frame* frame,
                                               word nargs) {
  Arguments args(frame, nargs);
  return Bool::fromBool(args.get(0).isRange());
}

RawObject UnderBuiltinsModule::underRangeGuard(Thread* thread, Frame* frame,
                                               word nargs) {
  Arguments args(frame, nargs);
  if (args.get(0).isRange()) {
    return NoneType::object();
  }
  return raiseRequiresFromCaller(thread, frame, nargs, SymbolId::kRange);
}

RawObject UnderBuiltinsModule::underRangeLen(Thread* thread, Frame* frame,
                                             word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Range self(&scope, args.get(0));
  Object start(&scope, self.start());
  Object stop(&scope, self.stop());
  Object step(&scope, self.step());
  return rangeLen(thread, start, stop, step);
}

RawObject UnderBuiltinsModule::underReprEnter(Thread* thread, Frame* frame,
                                              word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object obj(&scope, args.get(0));
  return thread->reprEnter(obj);
}

RawObject UnderBuiltinsModule::underReprLeave(Thread* thread, Frame* frame,
                                              word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object obj(&scope, args.get(0));
  thread->reprLeave(obj);
  return NoneType::object();
}

RawObject UnderBuiltinsModule::underSeqIndex(Thread* thread, Frame* frame,
                                             word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  SeqIterator self(&scope, args.get(0));
  return SmallInt::fromWord(self.index());
}

RawObject UnderBuiltinsModule::underSeqIterable(Thread* thread, Frame* frame,
                                                word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  SeqIterator self(&scope, args.get(0));
  return self.iterable();
}

RawObject UnderBuiltinsModule::underSeqSetIndex(Thread* thread, Frame* frame,
                                                word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  SeqIterator self(&scope, args.get(0));
  Int index(&scope, args.get(1));
  self.setIndex(index.asWord());
  return NoneType::object();
}

RawObject UnderBuiltinsModule::underSeqSetIterable(Thread* thread, Frame* frame,
                                                   word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  SeqIterator self(&scope, args.get(0));
  Object iterable(&scope, args.get(1));
  self.setIterable(*iterable);
  return NoneType::object();
}

RawObject UnderBuiltinsModule::underSetCheck(Thread* thread, Frame* frame,
                                             word nargs) {
  Arguments args(frame, nargs);
  return Bool::fromBool(thread->runtime()->isInstanceOfSet(args.get(0)));
}

RawObject UnderBuiltinsModule::underSetGuard(Thread* thread, Frame* frame,
                                             word nargs) {
  Arguments args(frame, nargs);
  if (thread->runtime()->isInstanceOfSet(args.get(0))) {
    return NoneType::object();
  }
  return raiseRequiresFromCaller(thread, frame, nargs, SymbolId::kSet);
}

RawObject UnderBuiltinsModule::underSetLen(Thread* thread, Frame* frame,
                                           word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Set self(&scope, args.get(0));
  return SmallInt::fromWord(self.numItems());
}

RawObject UnderBuiltinsModule::underSetMemberDouble(Thread*, Frame* frame,
                                                    word nargs) {
  Arguments args(frame, nargs);
  auto addr = Int::cast(args.get(0)).asCPtr();
  double value = Float::cast(args.get(1)).value();
  std::memcpy(reinterpret_cast<void*>(addr), &value, sizeof(value));
  return NoneType::object();
}

RawObject UnderBuiltinsModule::underSetMemberFloat(Thread*, Frame* frame,
                                                   word nargs) {
  Arguments args(frame, nargs);
  auto addr = Int::cast(args.get(0)).asCPtr();
  float value = Float::cast(args.get(1)).value();
  std::memcpy(reinterpret_cast<void*>(addr), &value, sizeof(value));
  return NoneType::object();
}

RawObject UnderBuiltinsModule::underSetMemberIntegral(Thread*, Frame* frame,
                                                      word nargs) {
  Arguments args(frame, nargs);
  auto addr = Int::cast(args.get(0)).asCPtr();
  auto value = RawInt::cast(args.get(1)).asWord();
  auto num_bytes = RawInt::cast(args.get(2)).asWord();
  std::memcpy(reinterpret_cast<void*>(addr), &value, num_bytes);
  return NoneType::object();
}

RawObject UnderBuiltinsModule::underSetMemberPyObject(Thread* thread,
                                                      Frame* frame,
                                                      word nargs) {
  Arguments args(frame, nargs);
  auto addr = Int::cast(args.get(0)).asCPtr();
  PyObject* value = ApiHandle::borrowedReference(thread, args.get(1));
  std::memcpy(reinterpret_cast<void*>(addr), &value, sizeof(value));
  return NoneType::object();
}

RawObject UnderBuiltinsModule::underSliceCheck(Thread*, Frame* frame,
                                               word nargs) {
  Arguments args(frame, nargs);
  return Bool::fromBool(args.get(0).isSlice());
}

RawObject UnderBuiltinsModule::underSliceGuard(Thread* thread, Frame* frame,
                                               word nargs) {
  Arguments args(frame, nargs);
  if (args.get(0).isSlice()) {
    return NoneType::object();
  }
  return raiseRequiresFromCaller(thread, frame, nargs, SymbolId::kSlice);
}

RawObject UnderBuiltinsModule::underSliceStart(Thread* thread, Frame* frame,
                                               word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object step_obj(&scope, args.get(1));
  Int step(&scope, intUnderlying(thread, step_obj));
  Object length_obj(&scope, args.get(2));
  Int length(&scope, intUnderlying(thread, length_obj));
  bool negative_step = step.isNegative();
  Int lower(&scope, SmallInt::fromWord(negative_step ? -1 : 0));
  Runtime* runtime = thread->runtime();
  // upper = length + lower; if step < 0, then lower = 0 anyway
  Int upper(&scope,
            negative_step ? runtime->intAdd(thread, length, lower) : *length);
  Object start_obj(&scope, args.get(0));
  if (start_obj.isNoneType()) {
    return negative_step ? *upper : *lower;
  }
  Int start(&scope, intUnderlying(thread, start_obj));
  if (start.isNegative()) {
    start = runtime->intAdd(thread, start, length);
    if (start.compare(*lower) < 0) {
      start = *lower;
    }
  } else if (start.compare(*upper) > 0) {
    start = *upper;
  }
  return *start;
}

RawObject UnderBuiltinsModule::underSliceStep(Thread* thread, Frame* frame,
                                              word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object step_obj(&scope, args.get(0));
  if (step_obj.isNoneType()) return SmallInt::fromWord(1);
  Int step(&scope, intUnderlying(thread, step_obj));
  if (step.isZero()) {
    return thread->raiseWithFmt(LayoutId::kValueError,
                                "slice step cannot be zero");
  }
  return *step;
}

RawObject UnderBuiltinsModule::underSliceStop(Thread* thread, Frame* frame,
                                              word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object step_obj(&scope, args.get(1));
  Int step(&scope, intUnderlying(thread, step_obj));
  Object length_obj(&scope, args.get(2));
  Int length(&scope, intUnderlying(thread, length_obj));
  bool negative_step = step.isNegative();
  Int lower(&scope, SmallInt::fromWord(negative_step ? -1 : 0));
  Runtime* runtime = thread->runtime();
  // upper = length + lower; if step < 0, then lower = 0 anyway
  Int upper(&scope,
            negative_step ? runtime->intAdd(thread, length, lower) : *length);
  Object stop_obj(&scope, args.get(0));
  if (stop_obj.isNoneType()) {
    return negative_step ? *lower : *upper;
  }
  Int stop(&scope, intUnderlying(thread, stop_obj));
  if (stop.isNegative()) {
    stop = runtime->intAdd(thread, stop, length);
    if (stop.compare(*lower) < 0) {
      stop = *lower;
    }
  } else if (stop.compare(*upper) > 0) {
    stop = *upper;
  }
  return *stop;
}

RawObject UnderBuiltinsModule::underStaticMethodIsAbstract(Thread* thread,
                                                           Frame* frame,
                                                           word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  StaticMethod self(&scope, args.get(0));
  Object func(&scope, self.function());
  return isAbstract(thread, func);
}

RawObject UnderBuiltinsModule::underStrArrayIadd(Thread* thread, Frame* frame,
                                                 word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  StrArray self(&scope, args.get(0));
  Object other_obj(&scope, args.get(1));
  Str other(&scope, strUnderlying(thread, other_obj));
  thread->runtime()->strArrayAddStr(thread, self, other);
  return *self;
}

RawObject UnderBuiltinsModule::underStrCheck(Thread* thread, Frame* frame,
                                             word nargs) {
  Arguments args(frame, nargs);
  return Bool::fromBool(thread->runtime()->isInstanceOfStr(args.get(0)));
}

RawObject UnderBuiltinsModule::underStrCheckExact(Thread*, Frame* frame,
                                                  word nargs) {
  Arguments args(frame, nargs);
  return Bool::fromBool(args.get(0).isStr());
}

RawObject UnderBuiltinsModule::underStrCount(Thread* thread, Frame* frame,
                                             word nargs) {
  Runtime* runtime = thread->runtime();
  Arguments args(frame, nargs);
  DCHECK(runtime->isInstanceOfStr(args.get(0)),
         "_str_count requires 'str' instance");
  DCHECK(runtime->isInstanceOfStr(args.get(1)),
         "_str_count requires 'str' instance");
  HandleScope scope(thread);
  Str haystack(&scope, args.get(0));
  Str needle(&scope, args.get(1));
  Object start_obj(&scope, args.get(2));
  Object end_obj(&scope, args.get(3));
  word start = 0;
  if (!start_obj.isNoneType()) {
    Int start_int(&scope, intUnderlying(thread, start_obj));
    start = start_int.asWordSaturated();
  }
  word end = kMaxWord;
  if (!end_obj.isNoneType()) {
    Int end_int(&scope, intUnderlying(thread, end_obj));
    end = end_int.asWordSaturated();
  }
  return strCount(haystack, needle, start, end);
}

RawObject UnderBuiltinsModule::underStrEscapeNonAscii(Thread* thread,
                                                      Frame* frame,
                                                      word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  CHECK(thread->runtime()->isInstanceOfStr(args.get(0)),
        "_str_escape_non_ascii expected str instance");
  Str obj(&scope, args.get(0));
  return strEscapeNonASCII(thread, obj);
}

RawObject UnderBuiltinsModule::underStrFind(Thread* thread, Frame* frame,
                                            word nargs) {
  Runtime* runtime = thread->runtime();
  Arguments args(frame, nargs);
  DCHECK(runtime->isInstanceOfStr(args.get(0)),
         "_str_find requires 'str' instance");
  DCHECK(runtime->isInstanceOfStr(args.get(1)),
         "_str_find requires 'str' instance");
  HandleScope scope(thread);
  Str haystack(&scope, args.get(0));
  Str needle(&scope, args.get(1));
  Object start_obj(&scope, args.get(2));
  Object end_obj(&scope, args.get(3));
  word start = 0;
  if (!start_obj.isNoneType()) {
    Int start_int(&scope, intUnderlying(thread, start_obj));
    start = start_int.asWordSaturated();
  }
  word end = kMaxWord;
  if (!end_obj.isNoneType()) {
    Int end_int(&scope, intUnderlying(thread, end_obj));
    end = end_int.asWordSaturated();
  }
  return strFind(haystack, needle, start, end);
}

RawObject UnderBuiltinsModule::underStrFromStr(Thread* thread, Frame* frame,
                                               word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Type type(&scope, args.get(0));
  DCHECK(type.builtinBase() == LayoutId::kStr, "type must subclass str");
  Object value_obj(&scope, args.get(1));
  Str value(&scope, strUnderlying(thread, value_obj));
  if (type.isBuiltin()) return *value;
  Layout type_layout(&scope, type.instanceLayout());
  UserStrBase instance(&scope, thread->runtime()->newInstance(type_layout));
  instance.setValue(*value);
  return *instance;
}

RawObject UnderBuiltinsModule::underStrGuard(Thread* thread, Frame* frame,
                                             word nargs) {
  Arguments args(frame, nargs);
  if (thread->runtime()->isInstanceOfStr(args.get(0))) {
    return NoneType::object();
  }
  return raiseRequiresFromCaller(thread, frame, nargs, SymbolId::kStr);
}

RawObject UnderBuiltinsModule::underStrJoin(Thread* thread, Frame* frame,
                                            word nargs) {
  Runtime* runtime = thread->runtime();
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Str sep(&scope, args.get(0));
  Object iterable(&scope, args.get(1));
  if (iterable.isTuple()) {
    Tuple tuple(&scope, *iterable);
    return runtime->strJoin(thread, sep, tuple, tuple.length());
  }
  DCHECK(iterable.isList(), "iterable must be tuple or list");
  List list(&scope, *iterable);
  Tuple tuple(&scope, list.items());
  return runtime->strJoin(thread, sep, tuple, list.numItems());
}

RawObject UnderBuiltinsModule::underStrLen(Thread* thread, Frame* frame,
                                           word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  Str self(&scope, strUnderlying(thread, self_obj));
  return SmallInt::fromWord(self.codePointLength());
}

RawObject UnderBuiltinsModule::underStrReplace(Thread* thread, Frame* frame,
                                               word nargs) {
  Runtime* runtime = thread->runtime();
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  Object oldstr_obj(&scope, args.get(1));
  Object newstr_obj(&scope, args.get(2));
  Str self(&scope, strUnderlying(thread, self_obj));
  Str oldstr(&scope, strUnderlying(thread, oldstr_obj));
  Str newstr(&scope, strUnderlying(thread, newstr_obj));
  Object count_obj(&scope, args.get(3));
  Int count(&scope, intUnderlying(thread, count_obj));
  return runtime->strReplace(thread, self, oldstr, newstr,
                             count.asWordSaturated());
}

RawObject UnderBuiltinsModule::underStrRFind(Thread* thread, Frame* frame,
                                             word nargs) {
  Runtime* runtime = thread->runtime();
  Arguments args(frame, nargs);
  DCHECK(runtime->isInstanceOfStr(args.get(0)),
         "_str_rfind requires 'str' instance");
  DCHECK(runtime->isInstanceOfStr(args.get(1)),
         "_str_rfind requires 'str' instance");
  HandleScope scope(thread);
  Str haystack(&scope, args.get(0));
  Str needle(&scope, args.get(1));
  Object start_obj(&scope, args.get(2));
  Object end_obj(&scope, args.get(3));
  word start = 0;
  if (!start_obj.isNoneType()) {
    Int start_int(&scope, intUnderlying(thread, start_obj));
    start = start_int.asWordSaturated();
  }
  word end = kMaxWord;
  if (!end_obj.isNoneType()) {
    Int end_int(&scope, intUnderlying(thread, end_obj));
    end = end_int.asWordSaturated();
  }
  return strRFind(haystack, needle, start, end);
}

RawObject UnderBuiltinsModule::underStrSplitlines(Thread* thread, Frame* frame,
                                                  word nargs) {
  Runtime* runtime = thread->runtime();
  Arguments args(frame, nargs);
  DCHECK(runtime->isInstanceOfStr(args.get(0)),
         "_str_splitlines requires 'str' instance");
  DCHECK(runtime->isInstanceOfInt(args.get(1)),
         "_str_splitlines requires 'int' instance");
  HandleScope scope(thread);
  Str self(&scope, args.get(0));
  Object keepends_obj(&scope, args.get(1));
  Int keepends_int(&scope, intUnderlying(thread, keepends_obj));
  bool keepends = !keepends_int.isZero();
  return strSplitlines(thread, self, keepends);
}

RawObject UnderBuiltinsModule::underTupleCheck(Thread* thread, Frame* frame,
                                               word nargs) {
  Arguments args(frame, nargs);
  return Bool::fromBool(thread->runtime()->isInstanceOfTuple(args.get(0)));
}

RawObject UnderBuiltinsModule::underTupleCheckExact(Thread*, Frame* frame,
                                                    word nargs) {
  Arguments args(frame, nargs);
  return Bool::fromBool(args.get(0).isTuple());
}

RawObject UnderBuiltinsModule::underTupleGuard(Thread* thread, Frame* frame,
                                               word nargs) {
  Arguments args(frame, nargs);
  if (thread->runtime()->isInstanceOfTuple(args.get(0))) {
    return NoneType::object();
  }
  return raiseRequiresFromCaller(thread, frame, nargs, SymbolId::kTuple);
}

RawObject UnderBuiltinsModule::underTupleLen(Thread* thread, Frame* frame,
                                             word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  Tuple self(&scope, tupleUnderlying(thread, self_obj));
  return SmallInt::fromWord(self.length());
}

RawObject UnderBuiltinsModule::underTupleNew(Thread* thread, Frame* frame,
                                             word nargs) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Type type(&scope, args.get(0));
  Object iterable(&scope, args.get(1));
  Tuple tuple(&scope, runtime->emptyTuple());
  if (runtime->isInstanceOfTuple(*iterable)) {
    tuple = tupleUnderlying(thread, iterable);
  } else {
    DCHECK(runtime->isInstanceOfList(*iterable),
           "iterable must be a tuple or a list");
    List list(&scope, *iterable);
    Tuple items(&scope, list.items());
    tuple = runtime->tupleSubseq(thread, items, 0, list.numItems());
  }
  if (type.isBuiltin()) return *tuple;
  Layout layout(&scope, type.instanceLayout());
  UserTupleBase instance(&scope, thread->runtime()->newInstance(layout));
  instance.setTupleValue(*tuple);
  return *instance;
}

RawObject UnderBuiltinsModule::underType(Thread* thread, Frame* frame,
                                         word nargs) {
  Arguments args(frame, nargs);
  return thread->runtime()->typeOf(args.get(0));
}

RawObject UnderBuiltinsModule::underTypeAbstractMethodsDel(Thread* thread,
                                                           Frame* frame,
                                                           word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Type type(&scope, args.get(0));
  if (type.abstractMethods().isUnbound()) {
    return thread->raiseWithId(LayoutId::kAttributeError,
                               SymbolId::kDunderAbstractMethods);
  }
  type.setAbstractMethods(Unbound::object());
  type.setFlagsAndBuiltinBase(
      static_cast<Type::Flag>(type.flags() & ~Type::Flag::kIsAbstract),
      type.builtinBase());
  return NoneType::object();
}

RawObject UnderBuiltinsModule::underTypeAbstractMethodsGet(Thread* thread,
                                                           Frame* frame,
                                                           word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Type type(&scope, args.get(0));
  Object methods(&scope, type.abstractMethods());
  if (!methods.isUnbound()) {
    return *methods;
  }
  return thread->raiseWithId(LayoutId::kAttributeError,
                             SymbolId::kDunderAbstractMethods);
}

RawObject UnderBuiltinsModule::underTypeAbstractMethodsSet(Thread* thread,
                                                           Frame* frame,
                                                           word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Type type(&scope, args.get(0));
  Object abstract(&scope, Interpreter::isTrue(thread, args.get(1)));
  if (abstract.isError()) return *abstract;
  type.setAbstractMethods(args.get(1));
  if (Bool::cast(*abstract).value()) {
    type.setFlagsAndBuiltinBase(
        static_cast<Type::Flag>(type.flags() | Type::Flag::kIsAbstract),
        type.builtinBase());
  }
  return NoneType::object();
}

RawObject UnderBuiltinsModule::underTypeBasesDel(Thread* thread, Frame* frame,
                                                 word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Type type(&scope, args.get(0));
  Str name(&scope, type.name());
  return thread->raiseWithFmt(LayoutId::kTypeError, "can't delete %S.__bases__",
                              &name);
}

RawObject UnderBuiltinsModule::underTypeBasesGet(Thread* thread, Frame* frame,
                                                 word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  return Type(&scope, args.get(0)).bases();
}

RawObject UnderBuiltinsModule::underTypeBasesSet(Thread*, Frame*, word) {
  UNIMPLEMENTED("type.__bases__ setter");
}

RawObject UnderBuiltinsModule::underTypeCheck(Thread* thread, Frame* frame,
                                              word nargs) {
  Arguments args(frame, nargs);
  return Bool::fromBool(thread->runtime()->isInstanceOfType(args.get(0)));
}

RawObject UnderBuiltinsModule::underTypeCheckExact(Thread*, Frame* frame,
                                                   word nargs) {
  Arguments args(frame, nargs);
  return Bool::fromBool(args.get(0).isType());
}

RawObject UnderBuiltinsModule::underTypeDictKeys(Thread* thread, Frame* frame,
                                                 word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Dict dict(&scope, args.get(0));
  Tuple data(&scope, dict.data());
  Runtime* runtime = thread->runtime();
  List keys(&scope, runtime->newList());
  Object key(&scope, NoneType::object());
  for (word i = Dict::Bucket::kFirst; Dict::Bucket::nextItem(*data, &i);) {
    RawObject value = Dict::Bucket::value(*data, i);
    DCHECK(value.isValueCell(), "values in type dict should be ValueCells");
    if (ValueCell::cast(value).isPlaceholder()) continue;
    key = Dict::Bucket::key(*data, i);
    runtime->listAdd(thread, keys, key);
  }
  return *keys;
}

RawObject UnderBuiltinsModule::underTypeGuard(Thread* thread, Frame* frame,
                                              word nargs) {
  Arguments args(frame, nargs);
  if (thread->runtime()->isInstanceOfType(args.get(0))) {
    return NoneType::object();
  }
  return raiseRequiresFromCaller(thread, frame, nargs, SymbolId::kType);
}

RawObject UnderBuiltinsModule::underTypeIsSubclass(Thread* thread, Frame* frame,
                                                   word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Type subclass(&scope, args.get(0));
  Type superclass(&scope, args.get(1));
  return Bool::fromBool(thread->runtime()->isSubclass(subclass, superclass));
}

RawObject UnderBuiltinsModule::underUnimplemented(Thread* thread, Frame* frame,
                                                  word) {
  python::Utils::printTracebackToStderr();

  // Attempt to identify the calling function.
  HandleScope scope(thread);
  Object function_obj(&scope, frame->previousFrame()->function());
  if (!function_obj.isError()) {
    Function function(&scope, *function_obj);
    Str function_name(&scope, function.name());
    unique_c_ptr<char> name_cstr(function_name.toCStr());
    fprintf(stderr, "\n'_unimplemented' called in function '%s'.\n",
            name_cstr.get());
  } else {
    fputs("\n'_unimplemented' called.\n", stderr);
  }

  std::abort();
}

}  // namespace python

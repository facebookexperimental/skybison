// typeobject.c implementation

#include "cpython-data.h"
#include "cpython-func.h"
#include "cpython-types.h"

#include <cinttypes>

#include "builtins-module.h"
#include "handles.h"
#include "int-builtins.h"
#include "mro.h"
#include "objects.h"
#include "runtime.h"
#include "trampolines.h"
#include "utils.h"

namespace python {

PY_EXPORT int PyType_CheckExact_Func(PyObject* obj) {
  return ApiHandle::fromPyObject(obj)->asObject().isType();
}

PY_EXPORT int PyType_Check_Func(PyObject* obj) {
  return Thread::current()->runtime()->isInstanceOfType(
      ApiHandle::fromPyObject(obj)->asObject());
}

static RawObject extensionSlot(const Type& type, Type::ExtensionSlot slot_id) {
  DCHECK(!type.extensionSlots().isNoneType(), "Type is not an extension");
  return Tuple::cast(type.extensionSlots()).at(static_cast<word>(slot_id));
}

static void setExtensionSlot(const Type& type, Type::ExtensionSlot slot_id,
                             RawObject slot) {
  DCHECK(!type.extensionSlots().isNoneType(), "Type is not an extension");
  return Tuple::cast(type.extensionSlots())
      .atPut(static_cast<word>(slot_id), slot);
}

PY_EXPORT unsigned long PyType_GetFlags(PyTypeObject* type_obj) {
  CHECK(ApiHandle::isManaged(reinterpret_cast<PyObject*>(type_obj)),
        "Type is unmanaged. Please initialize using PyType_FromSpec");

  HandleScope scope;
  Type type(&scope,
            ApiHandle::fromPyObject(reinterpret_cast<PyObject*>(type_obj))
                ->asObject());
  if (type.isBuiltin()) {
    UNIMPLEMENTED("GetFlags from built-in types");
  }

  if (type.extensionSlots().isNoneType()) {
    UNIMPLEMENTED("GetFlags from types initialized through Python code");
  }

  Int flags(&scope, extensionSlot(type, Type::ExtensionSlot::kFlags));
  return flags.asWord();
}

static Type::ExtensionSlot slotToTypeSlot(int slot) {
  switch (slot) {
    case Py_mp_ass_subscript:
      return Type::ExtensionSlot::kMapAssSubscript;
    case Py_mp_length:
      return Type::ExtensionSlot::kMapLength;
    case Py_mp_subscript:
      return Type::ExtensionSlot::kMapSubscript;
    case Py_nb_absolute:
      return Type::ExtensionSlot::kNumberAbsolute;
    case Py_nb_add:
      return Type::ExtensionSlot::kNumberAdd;
    case Py_nb_and:
      return Type::ExtensionSlot::kNumberAnd;
    case Py_nb_bool:
      return Type::ExtensionSlot::kNumberBool;
    case Py_nb_divmod:
      return Type::ExtensionSlot::kNumberDivmod;
    case Py_nb_float:
      return Type::ExtensionSlot::kNumberFloat;
    case Py_nb_floor_divide:
      return Type::ExtensionSlot::kNumberFloorDivide;
    case Py_nb_index:
      return Type::ExtensionSlot::kNumberIndex;
    case Py_nb_inplace_add:
      return Type::ExtensionSlot::kNumberInplaceAdd;
    case Py_nb_inplace_and:
      return Type::ExtensionSlot::kNumberInplaceAnd;
    case Py_nb_inplace_floor_divide:
      return Type::ExtensionSlot::kNumberInplaceFloorDivide;
    case Py_nb_inplace_lshift:
      return Type::ExtensionSlot::kNumberInplaceLshift;
    case Py_nb_inplace_multiply:
      return Type::ExtensionSlot::kNumberInplaceMultiply;
    case Py_nb_inplace_or:
      return Type::ExtensionSlot::kNumberInplaceOr;
    case Py_nb_inplace_power:
      return Type::ExtensionSlot::kNumberInplacePower;
    case Py_nb_inplace_remainder:
      return Type::ExtensionSlot::kNumberInplaceRemainder;
    case Py_nb_inplace_rshift:
      return Type::ExtensionSlot::kNumberInplaceRshift;
    case Py_nb_inplace_subtract:
      return Type::ExtensionSlot::kNumberInplaceSubtract;
    case Py_nb_inplace_true_divide:
      return Type::ExtensionSlot::kNumberInplaceTrueDivide;
    case Py_nb_inplace_xor:
      return Type::ExtensionSlot::kNumberInplaceXor;
    case Py_nb_int:
      return Type::ExtensionSlot::kNumberInt;
    case Py_nb_invert:
      return Type::ExtensionSlot::kNumberInvert;
    case Py_nb_lshift:
      return Type::ExtensionSlot::kNumberLshift;
    case Py_nb_multiply:
      return Type::ExtensionSlot::kNumberMultiply;
    case Py_nb_negative:
      return Type::ExtensionSlot::kNumberNegative;
    case Py_nb_or:
      return Type::ExtensionSlot::kNumberOr;
    case Py_nb_positive:
      return Type::ExtensionSlot::kNumberPositive;
    case Py_nb_power:
      return Type::ExtensionSlot::kNumberPower;
    case Py_nb_remainder:
      return Type::ExtensionSlot::kNumberRemainder;
    case Py_nb_rshift:
      return Type::ExtensionSlot::kNumberRshift;
    case Py_nb_subtract:
      return Type::ExtensionSlot::kNumberSubtract;
    case Py_nb_true_divide:
      return Type::ExtensionSlot::kNumberTrueDivide;
    case Py_nb_xor:
      return Type::ExtensionSlot::kNumberXor;
    case Py_sq_ass_item:
      return Type::ExtensionSlot::kSequenceAssItem;
    case Py_sq_concat:
      return Type::ExtensionSlot::kSequenceConcat;
    case Py_sq_contains:
      return Type::ExtensionSlot::kSequenceContains;
    case Py_sq_inplace_concat:
      return Type::ExtensionSlot::kSequenceInplaceConcat;
    case Py_sq_inplace_repeat:
      return Type::ExtensionSlot::kSequenceInplaceRepeat;
    case Py_sq_item:
      return Type::ExtensionSlot::kSequenceItem;
    case Py_sq_length:
      return Type::ExtensionSlot::kSequenceLength;
    case Py_sq_repeat:
      return Type::ExtensionSlot::kSequenceRepeat;
    case Py_tp_alloc:
      return Type::ExtensionSlot::kAlloc;
    case Py_tp_base:
      return Type::ExtensionSlot::kBase;
    case Py_tp_bases:
      return Type::ExtensionSlot::kBases;
    case Py_tp_call:
      return Type::ExtensionSlot::kCall;
    case Py_tp_clear:
      return Type::ExtensionSlot::kClear;
    case Py_tp_dealloc:
      return Type::ExtensionSlot::kDealloc;
    case Py_tp_del:
      return Type::ExtensionSlot::kDel;
    case Py_tp_descr_get:
      return Type::ExtensionSlot::kDescrGet;
    case Py_tp_descr_set:
      return Type::ExtensionSlot::kDescrSet;
    case Py_tp_doc:
      return Type::ExtensionSlot::kDoc;
    case Py_tp_getattr:
      return Type::ExtensionSlot::kGetattr;
    case Py_tp_getattro:
      return Type::ExtensionSlot::kGetattro;
    case Py_tp_hash:
      return Type::ExtensionSlot::kHash;
    case Py_tp_init:
      return Type::ExtensionSlot::kInit;
    case Py_tp_is_gc:
      return Type::ExtensionSlot::kIsGc;
    case Py_tp_iter:
      return Type::ExtensionSlot::kIter;
    case Py_tp_iternext:
      return Type::ExtensionSlot::kIternext;
    case Py_tp_methods:
      return Type::ExtensionSlot::kMethods;
    case Py_tp_new:
      return Type::ExtensionSlot::kNew;
    case Py_tp_repr:
      return Type::ExtensionSlot::kRepr;
    case Py_tp_richcompare:
      return Type::ExtensionSlot::kRichcompare;
    case Py_tp_setattr:
      return Type::ExtensionSlot::kSetattr;
    case Py_tp_setattro:
      return Type::ExtensionSlot::kSetattro;
    case Py_tp_str:
      return Type::ExtensionSlot::kStr;
    case Py_tp_traverse:
      return Type::ExtensionSlot::kTraverse;
    case Py_tp_members:
      return Type::ExtensionSlot::kMembers;
    case Py_tp_getset:
      return Type::ExtensionSlot::kGetset;
    case Py_tp_free:
      return Type::ExtensionSlot::kFree;
    case Py_nb_matrix_multiply:
      return Type::ExtensionSlot::kNumberMatrixMultiply;
    case Py_nb_inplace_matrix_multiply:
      return Type::ExtensionSlot::kNumberInplaceMatrixMultiply;
    case Py_am_await:
      return Type::ExtensionSlot::kAsyncAwait;
    case Py_am_aiter:
      return Type::ExtensionSlot::kAsyncAiter;
    case Py_am_anext:
      return Type::ExtensionSlot::kAsyncAnext;
    case Py_tp_finalize:
      return Type::ExtensionSlot::kFinalize;
    default:
      return Type::ExtensionSlot::kEnd;
  }
}

namespace {

// PyType_FromSpec() operator support
//
// The functions and data in this namespace, culminating in addOperators(), are
// used to add Python-visible wrappers for type slot C functions (e.g., passing
// a Py_nb_add slot will result in a __add__() method being added to the type).
// The wrapper functions (wrapUnaryfunc(), wrapBinaryfunc(), etc...) handle
// translating between incoming/outgoing RawObject/PyObject* values, along with
// various bits of slot-specific logic.
//
// The Function objects created in addOperators() have slotTrampoline*() (for
// most slots) or varkwSlotTrampoline*() (for variadic slots like Py_tp_call or
// Py_tp_init) as their entry points. The Function's Code object has a pointer
// to the appropriate wrapper function as its code field, and its consts field
// is a 1-element tuple containing a pointer to the slot function provided by
// the user. If this multi-step lookup ever becomes a performance problem, we
// can easily template the trampolines and/or the wrapper functions, but this
// keeps the code compact for now.

// Raise a TypeError containing the name of the current function and one of
// thevarious ways the number of arguments can be wrong.
RawObject raiseWrongArgsImpl(Thread* thread, const char* which, word argc,
                             word target) {
  HandleScope scope(thread);
  Function func(&scope, thread->currentFrame()->function());
  Str func_name(&scope, func.name());
  return thread->raiseWithFmt(LayoutId::kTypeError,
                              "'%S' expected %s %w arguments, got %w",
                              &func_name, which, target, argc);
}

RawObject raiseWrongArgs(Thread* thread, word argc, word target) {
  return raiseWrongArgsImpl(thread, "exactly", argc, target);
}

RawObject raiseTooManyArgs(Thread* thread, word argc, word limit) {
  return raiseWrongArgsImpl(thread, "at most", argc, limit);
}

RawObject raiseTooFewArgs(Thread* thread, word argc, word limit) {
  return raiseWrongArgsImpl(thread, "at least", argc, limit);
}

// Get an appropriately-typed function pointer out of the consts tuple in the
// Code object in the given Frame.
template <typename Func>
Func getNativeFunc(Thread* thread, Frame* frame) {
  HandleScope scope(thread);
  Code code(&scope, frame->code());
  Tuple consts(&scope, code.consts());
  DCHECK(consts.length() == 1, "Unexpected tuple length");
  Int raw_fn(&scope, consts.at(0));
  return bit_cast<Func>(raw_fn.asCPtr());
}

RawObject wrapUnaryfunc(Thread* thread, Frame* frame, word argc) {
  if (argc != 1) return raiseWrongArgs(thread, argc, 1);
  auto func = getNativeFunc<unaryfunc>(thread, frame);

  Arguments args(frame, argc);
  PyObject* o = ApiHandle::borrowedReference(thread, args.get(0));
  PyObject* result = func(o);
  return ApiHandle::stealReference(thread, result);
}

// Common work for hashfunc, lenfunc, and inquiry, all of which take a single
// PyObject* and return an integral value.
template <class cfunc, class RetFunc>
RawObject wrapIntegralfunc(Thread* thread, Frame* frame, word argc,
                           RetFunc ret) {
  if (argc != 1) return raiseWrongArgs(thread, argc, 1);
  auto func = getNativeFunc<cfunc>(thread, frame);

  Arguments args(frame, argc);
  PyObject* o = ApiHandle::borrowedReference(thread, args.get(0));
  auto result = func(o);
  if (result == -1 && thread->hasPendingException()) return Error::exception();
  return ret(result);
}

RawObject wrapHashfunc(Thread* thread, Frame* frame, word argc) {
  return wrapIntegralfunc<hashfunc>(
      thread, frame, argc,
      [thread](Py_hash_t hash) { return thread->runtime()->newInt(hash); });
}

RawObject wrapLenfunc(Thread* thread, Frame* frame, word argc) {
  return wrapIntegralfunc<lenfunc>(
      thread, frame, argc,
      [thread](Py_ssize_t len) { return thread->runtime()->newInt(len); });
}

RawObject wrapInquirypred(Thread* thread, Frame* frame, word argc) {
  return wrapIntegralfunc<inquiry>(
      thread, frame, argc, [](int result) { return Bool::fromBool(result); });
}

RawObject wrapBinaryfuncImpl(Thread* thread, Frame* frame, word argc,
                             bool swap) {
  if (argc != 2) return raiseWrongArgs(thread, argc, 2);
  auto func = getNativeFunc<binaryfunc>(thread, frame);

  Arguments args(frame, argc);
  PyObject* o1 = ApiHandle::borrowedReference(thread, args.get(swap ? 1 : 0));
  PyObject* o2 = ApiHandle::borrowedReference(thread, args.get(swap ? 0 : 1));
  return ApiHandle::stealReference(thread, func(o1, o2));
}

RawObject wrapBinaryfunc(Thread* thread, Frame* frame, word argc) {
  return wrapBinaryfuncImpl(thread, frame, argc, false);
}

RawObject wrapBinaryfuncSwapped(Thread* thread, Frame* frame, word argc) {
  return wrapBinaryfuncImpl(thread, frame, argc, false);
}

RawObject wrapTernaryfuncImpl(Thread* thread, Frame* frame, word argc,
                              bool swap) {
  if (argc < 2) return raiseTooFewArgs(thread, argc, 2);
  if (argc > 3) return raiseTooManyArgs(thread, argc, 3);
  auto func = getNativeFunc<ternaryfunc>(thread, frame);

  Arguments args(frame, argc);
  PyObject* self = ApiHandle::borrowedReference(thread, args.get(swap ? 1 : 0));
  PyObject* value =
      ApiHandle::borrowedReference(thread, args.get(swap ? 0 : 1));
  PyObject* mod = ApiHandle::borrowedReference(
      thread, argc >= 3 ? args.get(2) : NoneType::object());
  return ApiHandle::stealReference(thread, func(self, value, mod));
}

// wrapTernaryfunc() vs. wrapVarkwTernaryfunc():
// - wrapTernaryfunc(Swapped)(): Wraps a C function expecting exactly 3
//   normal arguments, with the 3rd argument defaulting to None.
// - wrapVarkwTernaryfunc(): Wraps a C function expecting a self argument, a
//   tuple of positional arguments and an optional dict of keyword arguments.
RawObject wrapTernaryfunc(Thread* thread, Frame* frame, word argc) {
  return wrapTernaryfuncImpl(thread, frame, argc, false);
}

RawObject wrapTernaryfuncSwapped(Thread* thread, Frame* frame, word argc) {
  return wrapTernaryfuncImpl(thread, frame, argc, true);
}

RawObject wrapVarkwTernaryfunc(Thread* thread, Frame* frame, word argc) {
  DCHECK(argc == 3, "Unexpected argc");
  auto func = getNativeFunc<ternaryfunc>(thread, frame);

  Arguments args(frame, argc);
  PyObject* self = ApiHandle::borrowedReference(thread, args.get(0));
  PyObject* varargs = ApiHandle::borrowedReference(thread, args.get(1));
  PyObject* kwargs = nullptr;
  if (!args.get(2).isNoneType()) {
    kwargs = ApiHandle::borrowedReference(thread, args.get(2));
  }
  return ApiHandle::stealReference(thread, func(self, varargs, kwargs));
}

RawObject wrapSetattr(Thread* thread, Frame* frame, word argc) {
  if (argc != 3) return raiseWrongArgs(thread, argc, 3);
  auto func = getNativeFunc<setattrofunc>(thread, frame);

  Arguments args(frame, argc);
  PyObject* self = ApiHandle::borrowedReference(thread, args.get(0));
  PyObject* name = ApiHandle::borrowedReference(thread, args.get(1));
  PyObject* value = ApiHandle::borrowedReference(thread, args.get(2));
  if (func(self, name, value) < 0) return Error::exception();
  return NoneType::object();
}

RawObject wrapDelattr(Thread* thread, Frame* frame, word argc) {
  if (argc != 2) return raiseWrongArgs(thread, argc, 2);
  auto func = getNativeFunc<setattrofunc>(thread, frame);

  Arguments args(frame, argc);
  PyObject* self = ApiHandle::borrowedReference(thread, args.get(0));
  PyObject* name = ApiHandle::borrowedReference(thread, args.get(1));
  if (func(self, name, nullptr) < 0) return Error::exception();
  return NoneType::object();
}

template <CompareOp op>
RawObject wrapRichcompare(Thread* thread, Frame* frame, word argc) {
  if (argc != 2) return raiseWrongArgs(thread, argc, 2);
  auto func = getNativeFunc<richcmpfunc>(thread, frame);

  Arguments args(frame, argc);
  PyObject* self = ApiHandle::borrowedReference(thread, args.get(0));
  PyObject* other = ApiHandle::borrowedReference(thread, args.get(1));
  return ApiHandle::stealReference(thread, func(self, other, op));
}

RawObject wrapNext(Thread* thread, Frame* frame, word argc) {
  if (argc != 1) return raiseWrongArgs(thread, argc, 1);
  auto func = getNativeFunc<unaryfunc>(thread, frame);

  Arguments args(frame, argc);
  PyObject* self = ApiHandle::borrowedReference(thread, args.get(0));
  PyObject* result = func(self);
  if (result == nullptr && !thread->hasPendingException()) {
    return thread->raise(LayoutId::kStopIteration, NoneType::object());
  }
  return ApiHandle::stealReference(thread, result);
}

RawObject wrapDescrGet(Thread* thread, Frame* frame, word argc) {
  if (argc < 2) return raiseTooFewArgs(thread, argc, 2);
  if (argc > 3) return raiseTooManyArgs(thread, argc, 3);
  auto func = getNativeFunc<descrgetfunc>(thread, frame);

  Arguments args(frame, argc);
  PyObject* self = ApiHandle::borrowedReference(thread, args.get(0));
  PyObject* obj = nullptr;
  if (!args.get(1).isNoneType()) {
    obj = ApiHandle::borrowedReference(thread, args.get(1));
  }
  PyObject* type = nullptr;
  if (argc >= 3 && !args.get(2).isNoneType()) {
    type = ApiHandle::borrowedReference(thread, args.get(2));
  }
  if (obj == nullptr && type == nullptr) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "__get__(None, None), is invalid");
  }
  return ApiHandle::stealReference(thread, func(self, obj, type));
}

RawObject wrapDescrSet(Thread* thread, Frame* frame, word argc) {
  if (argc != 3) return raiseWrongArgs(thread, argc, 3);
  auto func = getNativeFunc<descrsetfunc>(thread, frame);

  Arguments args(frame, argc);
  PyObject* self = ApiHandle::borrowedReference(thread, args.get(0));
  PyObject* obj = ApiHandle::borrowedReference(thread, args.get(1));
  PyObject* value = ApiHandle::borrowedReference(thread, args.get(2));
  if (func(self, obj, value) < 0) return Error::exception();
  return NoneType::object();
}

RawObject wrapDescrDelete(Thread* thread, Frame* frame, word argc) {
  if (argc != 2) return raiseWrongArgs(thread, argc, 2);
  auto func = getNativeFunc<descrsetfunc>(thread, frame);

  Arguments args(frame, argc);
  PyObject* self = ApiHandle::borrowedReference(thread, args.get(0));
  PyObject* obj = ApiHandle::borrowedReference(thread, args.get(1));
  if (func(self, obj, nullptr) < 0) return Error::exception();
  return NoneType::object();
}

RawObject wrapInit(Thread* thread, Frame* frame, word argc) {
  DCHECK(argc == 3, "Unexpected argc");
  auto func = getNativeFunc<initproc>(thread, frame);

  Arguments args(frame, argc);
  PyObject* self = ApiHandle::borrowedReference(thread, args.get(0));
  PyObject* varargs = ApiHandle::borrowedReference(thread, args.get(1));
  PyObject* kwargs = nullptr;
  if (!args.get(2).isNoneType()) {
    kwargs = ApiHandle::borrowedReference(thread, args.get(2));
  }
  if (func(self, varargs, kwargs) < 0) return Error::exception();
  return NoneType::object();
}

RawObject wrapDel(Thread* thread, Frame* frame, word argc) {
  if (argc != 1) return raiseWrongArgs(thread, argc, 1);
  auto func = getNativeFunc<destructor>(thread, frame);

  Arguments args(frame, argc);
  PyObject* self = ApiHandle::borrowedReference(thread, args.get(0));
  func(self);
  return NoneType::object();
}

RawObject wrapObjobjargproc(Thread* thread, Frame* frame, word argc) {
  if (argc != 3) return raiseWrongArgs(thread, argc, 3);
  auto func = getNativeFunc<objobjargproc>(thread, frame);

  Arguments args(frame, argc);
  PyObject* self = ApiHandle::borrowedReference(thread, args.get(0));
  PyObject* key = ApiHandle::borrowedReference(thread, args.get(1));
  PyObject* value = ApiHandle::borrowedReference(thread, args.get(2));
  int res = func(self, key, value);
  if (res == -1 && thread->hasPendingException()) return Error::exception();
  return NoneType::object();
}

RawObject wrapObjobjproc(Thread* thread, Frame* frame, word argc) {
  if (argc != 2) return raiseWrongArgs(thread, argc, 2);
  auto func = getNativeFunc<objobjproc>(thread, frame);

  Arguments args(frame, argc);
  PyObject* self = ApiHandle::borrowedReference(thread, args.get(0));
  PyObject* value = ApiHandle::borrowedReference(thread, args.get(1));
  int res = func(self, value);
  if (res == -1 && thread->hasPendingException()) return Error::exception();
  return Bool::fromBool(res);
}

RawObject wrapDelitem(Thread* thread, Frame* frame, word argc) {
  if (argc != 2) return raiseWrongArgs(thread, argc, 2);
  auto func = getNativeFunc<objobjargproc>(thread, frame);

  Arguments args(frame, argc);
  PyObject* self = ApiHandle::borrowedReference(thread, args.get(0));
  PyObject* key = ApiHandle::borrowedReference(thread, args.get(1));
  int res = func(self, key, nullptr);
  if (res == -1 && thread->hasPendingException()) return Error::exception();
  return NoneType::object();
}

// Convert obj into a word-sized int or raise an OverflowError, in the style of
// PyNumber_AsSsize_t().
static RawObject makeIndex(Thread* thread, const Object& obj) {
  HandleScope scope(thread);
  Object converted(&scope, intFromIndex(thread, obj));
  if (converted.isError()) return *converted;
  Int i(&scope, intUnderlying(thread, converted));
  if (i.numDigits() != 1) {
    return thread->raiseWithFmt(LayoutId::kOverflowError,
                                "cannot fit '%T' into an index-sized integer",
                                &obj);
  }
  return *i;
}

RawObject wrapIndexargfunc(Thread* thread, Frame* frame, word argc) {
  if (argc != 2) return raiseWrongArgs(thread, argc, 2);
  auto func = getNativeFunc<ssizeargfunc>(thread, frame);

  HandleScope scope(thread);
  Arguments args(frame, argc);
  PyObject* self = ApiHandle::borrowedReference(thread, args.get(0));
  Object arg(&scope, args.get(1));
  arg = makeIndex(thread, arg);
  if (arg.isError()) return *arg;
  return ApiHandle::stealReference(thread,
                                   func(self, Int::cast(*arg).asWord()));
}

// First, convert arg to a word-sized int using makeIndex(). Then, if the result
// is negative, add len(self) to normalize it.
static RawObject normalizeIndex(Thread* thread, const Object& self,
                                const Object& arg) {
  HandleScope scope(thread);
  Object index(&scope, makeIndex(thread, arg));
  if (index.isError()) return *index;
  word i = Int::cast(*index).asWord();
  if (i >= 0) {
    return *index;
  }
  Object len(&scope, thread->invokeFunction1(SymbolId::kBuiltins,
                                             SymbolId::kLen, self));
  if (len.isError()) return *len;
  len = makeIndex(thread, len);
  if (len.isError()) return *len;
  i += Int::cast(*len).asWord();
  return thread->runtime()->newInt(i);
}

RawObject wrapSqItem(Thread* thread, Frame* frame, word argc) {
  if (argc != 2) return raiseWrongArgs(thread, argc, 2);
  auto func = getNativeFunc<ssizeargfunc>(thread, frame);

  HandleScope scope(thread);
  Arguments args(frame, argc);
  Object self(&scope, args.get(0));
  Object arg(&scope, args.get(1));
  arg = normalizeIndex(thread, self, arg);
  if (arg.isError()) return *arg;
  PyObject* py_self = ApiHandle::borrowedReference(thread, *self);
  return ApiHandle::stealReference(thread,
                                   func(py_self, Int::cast(*arg).asWord()));
}

RawObject wrapSqSetitem(Thread* thread, Frame* frame, word argc) {
  if (argc != 3) return raiseWrongArgs(thread, argc, 3);
  auto func = getNativeFunc<ssizeobjargproc>(thread, frame);

  HandleScope scope(thread);
  Arguments args(frame, argc);
  Object self(&scope, args.get(0));
  Object arg(&scope, args.get(1));
  arg = normalizeIndex(thread, self, arg);
  if (arg.isError()) return *arg;
  PyObject* py_self = ApiHandle::borrowedReference(thread, *self);
  PyObject* py_value = ApiHandle::borrowedReference(thread, args.get(2));
  int result = func(py_self, Int::cast(*arg).asWord(), py_value);
  if (result == -1 && thread->hasPendingException()) return Error::exception();
  return NoneType::object();
}

RawObject wrapSqDelitem(Thread* thread, Frame* frame, word argc) {
  if (argc != 2) return raiseWrongArgs(thread, argc, 2);
  auto func = getNativeFunc<ssizeobjargproc>(thread, frame);

  HandleScope scope(thread);
  Arguments args(frame, argc);
  Object self(&scope, args.get(0));
  Object arg(&scope, args.get(1));
  arg = normalizeIndex(thread, self, arg);
  if (arg.isError()) return *arg;
  PyObject* py_self = ApiHandle::borrowedReference(thread, *self);
  int result = func(py_self, Int::cast(*arg).asWord(), nullptr);
  if (result == -1 && thread->hasPendingException()) return Error::exception();
  return NoneType::object();
}

// Information about a single type slot.
struct SlotDef {
  // The name of the method in managed code.
  SymbolId name;

  // Our equivalent of the slot id from PyType_Slot.
  Type::ExtensionSlot id;

  // The wrapper function for this slot.
  Function::Entry wrapper;

  // True if and only if the function expect varargs and varkwargs.
  bool is_varkw;

  // Doc string for the function.
  const char* doc;
};

// These macros currently ignore the FUNCTION argument, which is still the
// function name inherited from CPython. This will be cleaned up when we add
// default slot implementations that delegate to the corresponding Python
// method, along with logic to update slots as needed when a user assigns to a
// type dict.
#define TPSLOT(NAME, SLOT, FUNCTION, WRAPPER, DOC)                             \
  { SymbolId::NAME, Type::ExtensionSlot::SLOT, WRAPPER, false, DOC }
#define KWSLOT(NAME, SLOT, FUNCTION, WRAPPER, DOC)                             \
  { SymbolId::NAME, Type::ExtensionSlot::SLOT, WRAPPER, true, DOC }
#define UNSLOT(NAME, C_NAME, SLOT, FUNCTION, WRAPPER, DOC)                     \
  TPSLOT(NAME, SLOT, FUNCTION, WRAPPER, C_NAME "($self, /)\n--\n\n" DOC)
#define IBSLOT(NAME, C_NAME, SLOT, FUNCTION, WRAPPER, DOC)                     \
  TPSLOT(NAME, SLOT, FUNCTION, WRAPPER,                                        \
         C_NAME "($self, value, /)\n--\n\nReturn self" DOC "value.")
#define BINSLOT(NAME, C_NAME, SLOT, FUNCTION, DOC)                             \
  TPSLOT(NAME, SLOT, FUNCTION, wrapBinaryfunc,                                 \
         C_NAME "($self, value, /)\n--\n\nReturn self" DOC "value.")
#define RBINSLOT(NAME, C_NAME, SLOT, FUNCTION, DOC)                            \
  TPSLOT(NAME, SLOT, FUNCTION, wrapBinaryfuncSwapped,                          \
         C_NAME "($self, value, /)\n--\n\nReturn value" DOC "self.")
#define BINSLOTNOTINFIX(NAME, C_NAME, SLOT, FUNCTION, DOC)                     \
  TPSLOT(NAME, SLOT, FUNCTION, wrapBinaryfunc,                                 \
         C_NAME "($self, value, /)\n--\n\n" DOC)
#define RBINSLOTNOTINFIX(NAME, C_NAME, SLOT, FUNCTION, DOC)                    \
  TPSLOT(NAME, SLOT, FUNCTION, wrapBinaryfuncSwapped,                          \
         C_NAME "($self, value, /)\n--\n\n" DOC)

const SlotDef kSlotdefs[] = {
    TPSLOT(kDunderGetattribute, kGetattr, nullptr, nullptr, ""),
    TPSLOT(kDunderGetattr, kGetattr, nullptr, nullptr, ""),
    TPSLOT(kDunderSetattr, kSetattr, nullptr, nullptr, ""),
    TPSLOT(kDunderDelattr, kSetattr, nullptr, nullptr, ""),
    TPSLOT(kDunderRepr, kRepr, slot_tp_repr, wrapUnaryfunc,
           "__repr__($self, /)\n--\n\nReturn repr(self)."),
    TPSLOT(kDunderHash, kHash, slot_tp_hash, wrapHashfunc,
           "__hash__($self, /)\n--\n\nReturn hash(self)."),
    KWSLOT(
        kDunderCall, kCall, slot_tp_call, wrapVarkwTernaryfunc,
        "__call__($self, /, *args, **kwargs)\n--\n\nCall self as a function."),
    TPSLOT(kDunderStr, kStr, slot_tp_str, wrapUnaryfunc,
           "__str__($self, /)\n--\n\nReturn str(self)."),
    TPSLOT(kDunderGetattribute, kGetattro, slot_tp_getattr_hook, wrapBinaryfunc,
           "__getattribute__($self, name, /)\n--\n\nReturn getattr(self, "
           "name)."),
    TPSLOT(kDunderGetattr, kGetattro, slot_tp_getattr_hook, nullptr, ""),
    TPSLOT(kDunderSetattr, kSetattro, slot_tp_setattro, wrapSetattr,
           "__setattr__($self, name, value, /)\n--\n\nImplement setattr(self, "
           "name, value)."),
    TPSLOT(kDunderDelattr, kSetattro, slot_tp_setattro, wrapDelattr,
           "__delattr__($self, name, /)\n--\n\nImplement delattr(self, name)."),
    TPSLOT(kDunderLt, kRichcompare, slot_tp_richcompare, wrapRichcompare<LT>,
           "__lt__($self, value, /)\n--\n\nReturn self<value."),
    TPSLOT(kDunderLe, kRichcompare, slot_tp_richcompare, wrapRichcompare<LE>,
           "__le__($self, value, /)\n--\n\nReturn self<=value."),
    TPSLOT(kDunderEq, kRichcompare, slot_tp_richcompare, wrapRichcompare<EQ>,
           "__eq__($self, value, /)\n--\n\nReturn self==value."),
    TPSLOT(kDunderNe, kRichcompare, slot_tp_richcompare, wrapRichcompare<NE>,
           "__ne__($self, value, /)\n--\n\nReturn self!=value."),
    TPSLOT(kDunderGt, kRichcompare, slot_tp_richcompare, wrapRichcompare<GT>,
           "__gt__($self, value, /)\n--\n\nReturn self>value."),
    TPSLOT(kDunderGe, kRichcompare, slot_tp_richcompare, wrapRichcompare<GE>,
           "__ge__($self, value, /)\n--\n\nReturn self>=value."),
    TPSLOT(kDunderIter, kIter, slot_tp_iter, wrapUnaryfunc,
           "__iter__($self, /)\n--\n\nImplement iter(self)."),
    TPSLOT(kDunderNext, kIternext, slot_tp_iternext, wrapNext,
           "__next__($self, /)\n--\n\nImplement next(self)."),
    TPSLOT(kDunderGet, kDescrGet, slot_tp_descr_get, wrapDescrGet,
           "__get__($self, instance, owner, /)\n--\n\nReturn an attribute of "
           "instance, which is of type owner."),
    TPSLOT(kDunderSet, kDescrSet, slot_tp_descr_set, wrapDescrSet,
           "__set__($self, instance, value, /)\n--\n\nSet an attribute of "
           "instance to value."),
    TPSLOT(kDunderDelete, kDescrSet, slot_tp_descr_set, wrapDescrDelete,
           "__delete__($self, instance, /)\n--\n\nDelete an attribute of "
           "instance."),
    KWSLOT(kDunderInit, kInit, slot_tp_init, wrapInit,
           "__init__($self, /, *args, **kwargs)\n--\n\nInitialize self.  See "
           "help(type(self)) for accurate signature."),
    KWSLOT(kDunderNew, kNew, slot_tp_new, wrapVarkwTernaryfunc,
           "__new__(type, /, *args, **kwargs)\n--\n\n"
           "Create and return new object.  See help(type) for accurate "
           "signature."),
    TPSLOT(kDunderDel, kFinalize, slot_tp_finalize, wrapDel, ""),
    TPSLOT(kDunderAwait, kAsyncAwait, slot_am_await, wrapUnaryfunc,
           "__await__($self, /)\n--\n\nReturn an iterator to be used in await "
           "expression."),
    TPSLOT(kDunderAiter, kAsyncAiter, slot_am_aiter, wrapUnaryfunc,
           "__aiter__($self, /)\n--\n\nReturn an awaitable, that resolves in "
           "asynchronous iterator."),
    TPSLOT(kDunderAnext, kAsyncAnext, slot_am_anext, wrapUnaryfunc,
           "__anext__($self, /)\n--\n\nReturn a value or raise "
           "StopAsyncIteration."),
    BINSLOT(kDunderAdd, "__add__", kNumberAdd, slot_nb_add, "+"),
    RBINSLOT(kDunderRadd, "__radd__", kNumberAdd, slot_nb_add, "+"),
    BINSLOT(kDunderSub, "__sub__", kNumberSubtract, slot_nb_subtract, "-"),
    RBINSLOT(kDunderRsub, "__rsub__", kNumberSubtract, slot_nb_subtract, "-"),
    BINSLOT(kDunderMul, "__mul__", kNumberMultiply, slot_nb_multiply, "*"),
    RBINSLOT(kDunderRmul, "__rmul__", kNumberMultiply, slot_nb_multiply, "*"),
    BINSLOT(kDunderMod, "__mod__", kNumberRemainder, slot_nb_remainder, "%"),
    RBINSLOT(kDunderRmod, "__rmod__", kNumberRemainder, slot_nb_remainder, "%"),
    BINSLOTNOTINFIX(kDunderDivmod, "__divmod__", kNumberDivmod, slot_nb_divmod,
                    "Return divmod(self, value)."),
    RBINSLOTNOTINFIX(kDunderRdivmod, "__rdivmod__", kNumberDivmod,
                     slot_nb_divmod, "Return divmod(value, self)."),
    TPSLOT(kDunderPow, kNumberPower, slot_nb_power, wrapTernaryfunc,
           "__pow__($self, value, mod=None, /)\n--\n\nReturn pow(self, value, "
           "mod)."),
    TPSLOT(kDunderRpow, kNumberPower, slot_nb_power, wrapTernaryfuncSwapped,
           "__rpow__($self, value, mod=None, /)\n--\n\nReturn pow(value, self, "
           "mod)."),
    UNSLOT(kDunderNeg, "__neg__", kNumberNegative, slot_nb_negative,
           wrapUnaryfunc, "-self"),
    UNSLOT(kDunderPos, "__pos__", kNumberPositive, slot_nb_positive,
           wrapUnaryfunc, "+self"),
    UNSLOT(kDunderAbs, "__abs__", kNumberAbsolute, slot_nb_absolute,
           wrapUnaryfunc, "abs(self)"),
    UNSLOT(kDunderBool, "__bool__", kNumberBool, slot_nb_bool, wrapInquirypred,
           "self != 0"),
    UNSLOT(kDunderInvert, "__invert__", kNumberInvert, slot_nb_invert,
           wrapUnaryfunc, "~self"),
    BINSLOT(kDunderLshift, "__lshift__", kNumberLshift, slot_nb_lshift, "<<"),
    RBINSLOT(kDunderRlshift, "__rlshift__", kNumberLshift, slot_nb_lshift,
             "<<"),
    BINSLOT(kDunderRshift, "__rshift__", kNumberRshift, slot_nb_rshift, ">>"),
    RBINSLOT(kDunderRrshift, "__rrshift__", kNumberRshift, slot_nb_rshift,
             ">>"),
    BINSLOT(kDunderAnd, "__and__", kNumberAnd, slot_nb_and, "&"),
    RBINSLOT(kDunderRand, "__rand__", kNumberAnd, slot_nb_and, "&"),
    BINSLOT(kDunderXor, "__xor__", kNumberXor, slot_nb_xor, "^"),
    RBINSLOT(kDunderRxor, "__rxor__", kNumberXor, slot_nb_xor, "^"),
    BINSLOT(kDunderOr, "__or__", kNumberOr, slot_nb_or, "|"),
    RBINSLOT(kDunderRor, "__ror__", kNumberOr, slot_nb_or, "|"),
    UNSLOT(kDunderInt, "__int__", kNumberInt, slot_nb_int, wrapUnaryfunc,
           "int(self)"),
    UNSLOT(kDunderFloat, "__float__", kNumberFloat, slot_nb_float,
           wrapUnaryfunc, "float(self)"),
    IBSLOT(kDunderIadd, "__iadd__", kNumberInplaceAdd, slot_nb_inplace_add,
           wrapBinaryfunc, "+="),
    IBSLOT(kDunderIsub, "__isub__", kNumberInplaceSubtract,
           slot_nb_inplace_subtract, wrapBinaryfunc, "-="),
    IBSLOT(kDunderImul, "__imul__", kNumberInplaceMultiply,
           slot_nb_inplace_multiply, wrapBinaryfunc, "*="),
    IBSLOT(kDunderImod, "__imod__", kNumberInplaceRemainder,
           slot_nb_inplace_remainder, wrapBinaryfunc, "%="),
    IBSLOT(kDunderIpow, "__ipow__", kNumberInplacePower, slot_nb_inplace_power,
           wrapBinaryfunc, "**="),
    IBSLOT(kDunderIlshift, "__ilshift__", kNumberInplaceLshift,
           slot_nb_inplace_lshift, wrapBinaryfunc, "<<="),
    IBSLOT(kDunderIrshift, "__irshift__", kNumberInplaceRshift,
           slot_nb_inplace_rshift, wrapBinaryfunc, ">>="),
    IBSLOT(kDunderIand, "__iand__", kNumberInplaceAnd, slot_nb_inplace_and,
           wrapBinaryfunc, "&="),
    IBSLOT(kDunderIxor, "__ixor__", kNumberInplaceXor, slot_nb_inplace_xor,
           wrapBinaryfunc, "^="),
    IBSLOT(kDunderIor, "__ior__", kNumberInplaceOr, slot_nb_inplace_or,
           wrapBinaryfunc, "|="),
    BINSLOT(kDunderFloordiv, "__floordiv__", kNumberFloorDivide,
            slot_nb_floor_divide, "//"),
    RBINSLOT(kDunderRfloordiv, "__rfloordiv__", kNumberFloorDivide,
             slot_nb_floor_divide, "//"),
    BINSLOT(kDunderTruediv, "__truediv__", kNumberTrueDivide,
            slot_nb_true_divide, "/"),
    RBINSLOT(kDunderRtruediv, "__rtruediv__", kNumberTrueDivide,
             slot_nb_true_divide, "/"),
    IBSLOT(kDunderIfloordiv, "__ifloordiv__", kNumberInplaceFloorDivide,
           slot_nb_inplace_floor_divide, wrapBinaryfunc, "//="),
    IBSLOT(kDunderItruediv, "__itruediv__", kNumberInplaceTrueDivide,
           slot_nb_inplace_true_divide, wrapBinaryfunc, "/="),
    TPSLOT(kDunderIndex, kNumberIndex, slot_nb_index, wrapUnaryfunc,
           "__index__($self, /)\n--\n\n"
           "Return self converted to an integer, if self is suitable "
           "for use as an index into a list."),
    BINSLOT(kDunderMatmul, "__matmul__", kNumberMatrixMultiply,
            slot_nb_matrix_multiply, "@"),
    RBINSLOT(kDunderRmatmul, "__rmatmul__", kNumberMatrixMultiply,
             slot_nb_matrix_multiply, "@"),
    IBSLOT(kDunderImatmul, "__imatmul__", kNumberInplaceMatrixMultiply,
           slot_nb_inplace_matrix_multiply, wrapBinaryfunc, "@="),
    TPSLOT(kDunderLen, kMapLength, slot_mp_length, wrapLenfunc,
           "__len__($self, /)\n--\n\nReturn len(self)."),
    TPSLOT(kDunderGetitem, kMapSubscript, slot_mp_subscript, wrapBinaryfunc,
           "__getitem__($self, key, /)\n--\n\nReturn self[key]."),
    TPSLOT(kDunderSetitem, kMapAssSubscript, slot_mp_ass_subscript,
           wrapObjobjargproc,
           "__setitem__($self, key, value, /)\n--\n\nSet self[key] to value."),
    TPSLOT(kDunderDelitem, kMapAssSubscript, slot_mp_ass_subscript, wrapDelitem,
           "__delitem__($self, key, /)\n--\n\nDelete self[key]."),
    TPSLOT(kDunderLen, kSequenceLength, slot_sq_length, wrapLenfunc,
           "__len__($self, /)\n--\n\nReturn len(self)."),
    TPSLOT(kDunderAdd, kSequenceConcat, nullptr, wrapBinaryfunc,
           "__add__($self, value, /)\n--\n\nReturn self+value."),
    TPSLOT(kDunderMul, kSequenceRepeat, nullptr, wrapIndexargfunc,
           "__mul__($self, value, /)\n--\n\nReturn self*value."),
    TPSLOT(kDunderRmul, kSequenceRepeat, nullptr, wrapIndexargfunc,
           "__rmul__($self, value, /)\n--\n\nReturn value*self."),
    TPSLOT(kDunderGetitem, kSequenceItem, slot_sq_item, wrapSqItem,
           "__getitem__($self, key, /)\n--\n\nReturn self[key]."),
    TPSLOT(kDunderSetitem, kSequenceAssItem, slot_sq_ass_item, wrapSqSetitem,
           "__setitem__($self, key, value, /)\n--\n\nSet self[key] to value."),
    TPSLOT(kDunderDelitem, kSequenceAssItem, slot_sq_ass_item, wrapSqDelitem,
           "__delitem__($self, key, /)\n--\n\nDelete self[key]."),
    TPSLOT(kDunderContains, kSequenceContains, slot_sq_contains, wrapObjobjproc,
           "__contains__($self, key, /)\n--\n\nReturn key in self."),
    TPSLOT(kDunderIadd, kSequenceInplaceConcat, nullptr, wrapBinaryfunc,
           "__iadd__($self, value, /)\n--\n\nImplement self+=value."),
    TPSLOT(kDunderImul, kSequenceInplaceRepeat, nullptr, wrapIndexargfunc,
           "__imul__($self, value, /)\n--\n\nImplement self*=value."),
};

// For every entry in kSlotdefs with a non-null wrapper function, a slot id
// that was provided by the user, and no preexisting entry in the type dict, add
// a wrapper function to call the slot from Python.
//
// Returns Error if an exception was raised at any point, None otherwise.
RawObject addOperators(Thread* thread, const Type& type) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Dict dict(&scope, type.dict());
  Str type_name(&scope, type.name());

  for (const SlotDef& slot : kSlotdefs) {
    if (slot.wrapper == nullptr) continue;
    Object slot_value(&scope, extensionSlot(type, slot.id));
    if (slot_value.isNoneType()) continue;
    DCHECK(slot_value.isInt(), "unexpected slot type");

    Str slot_name(&scope, runtime->symbols()->at(slot.name));
    if (!runtime->typeDictAt(thread, dict, slot_name).isError()) continue;

    // When given PyObject_HashNotImplemented, put None in the type dict
    // rather than a wrapper. CPython does this regardless of which slot it
    // was given for, so we do too.
    if (Int::cast(*slot_value).asCPtr() ==
        bit_cast<void*>(&PyObject_HashNotImplemented)) {
      Object none(&scope, NoneType::object());
      runtime->typeDictAtPut(thread, dict, slot_name, none);
      return NoneType::object();
    }

    // Create the wrapper function.
    Str qualname(&scope,
                 runtime->newStrFromFmt("%S.%S", &type_name, &slot_name));
    Code code(&scope, runtime->newEmptyCode(slot_name));
    code.setCode(runtime->newIntFromCPtr(bit_cast<void*>(slot.wrapper)));
    Tuple consts(&scope, runtime->newTuple(1));
    consts.atPut(0, *slot_value);
    code.setConsts(*consts);
    Function func(
        &scope, runtime->newNativeFunction(
                    slot.name, qualname,
                    slot.is_varkw ? varkwSlotTrampoline : slotTrampoline,
                    slot.is_varkw ? varkwSlotTrampolineKw : slotTrampolineKw,
                    slot.is_varkw ? varkwSlotTrampolineEx : slotTrampolineEx));
    func.setCode(*code);

    // __new__ is the one special-case static method, so wrap it
    // appropriately.
    Object func_obj(&scope, *func);
    if (slot.id == Type::ExtensionSlot::kNew) {
      func_obj = thread->invokeFunction1(SymbolId::kBuiltins,
                                         SymbolId::kStaticMethod, func);
      if (func_obj.isError()) return *func_obj;
    }

    // Finally, put the wrapper in the type dict.
    runtime->typeDictAtPut(thread, dict, slot_name, func_obj);
  }

  return NoneType::object();
}

}  // namespace

PY_EXPORT void* PyType_GetSlot(PyTypeObject* type_obj, int slot) {
  Thread* thread = Thread::current();
  if (slot < 0) {
    thread->raiseBadInternalCall();
    return nullptr;
  }

  if (!ApiHandle::isManaged(reinterpret_cast<PyObject*>(type_obj))) {
    thread->raiseBadInternalCall();
    return nullptr;
  }

  HandleScope scope(thread);
  Type type(&scope,
            ApiHandle::fromPyObject(reinterpret_cast<PyObject*>(type_obj))
                ->asObject());
  if (type.isBuiltin()) {
    thread->raiseBadInternalCall();
    return nullptr;
  }

  // Extension module requesting slot from a future version
  Type::ExtensionSlot field_id = slotToTypeSlot(slot);
  if (field_id >= Type::ExtensionSlot::kEnd) {
    return nullptr;
  }

  if (type.extensionSlots().isNoneType()) {
    UNIMPLEMENTED("Get slots from types initialized through Python code");
  }

  DCHECK(!type.extensionSlots().isNoneType(), "Type is not extension type");
  Int address(&scope, extensionSlot(type, field_id));
  return address.asCPtr();
}

PY_EXPORT int PyType_Ready(PyTypeObject*) {
  UNIMPLEMENTED("This function will never be implemented");
}

PY_EXPORT PyObject* PyType_FromSpec(PyType_Spec* spec) {
  return PyType_FromSpecWithBases(spec, nullptr);
}

static RawObject memberGetter(Thread* thread, PyMemberDef& member) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object name(&scope, runtime->newStrFromCStr(member.name));
  Tuple consts(&scope, runtime->newTuple(1));
  consts.atPut(0, runtime->newInt(member.offset));
  Int offset(&scope, runtime->newInt(member.offset));
  switch (member.type) {
    case T_BOOL:
      return thread->invokeFunction1(SymbolId::kBuiltins,
                                     SymbolId::kUnderNewMemberGetBool, offset);
    case T_BYTE:
      return thread->invokeFunction1(SymbolId::kBuiltins,
                                     SymbolId::kUnderNewMemberGetByte, offset);
    case T_UBYTE:
      return thread->invokeFunction1(SymbolId::kBuiltins,
                                     SymbolId::kUnderNewMemberGetUByte, offset);
    case T_SHORT:
      return thread->invokeFunction1(SymbolId::kBuiltins,
                                     SymbolId::kUnderNewMemberGetShort, offset);
    case T_USHORT:
      return thread->invokeFunction1(
          SymbolId::kBuiltins, SymbolId::kUnderNewMemberGetUShort, offset);
    case T_INT:
      return thread->invokeFunction1(SymbolId::kBuiltins,
                                     SymbolId::kUnderNewMemberGetInt, offset);
    case T_UINT:
      return thread->invokeFunction1(SymbolId::kBuiltins,
                                     SymbolId::kUnderNewMemberGetUInt, offset);
    case T_LONG:
      return thread->invokeFunction1(SymbolId::kBuiltins,
                                     SymbolId::kUnderNewMemberGetLong, offset);
    case T_ULONG:
      return thread->invokeFunction1(SymbolId::kBuiltins,
                                     SymbolId::kUnderNewMemberGetULong, offset);
    case T_PYSSIZET:
      return thread->invokeFunction1(SymbolId::kBuiltins,
                                     SymbolId::kUnderNewMemberGetULong, offset);
    case T_FLOAT:
      return thread->invokeFunction1(SymbolId::kBuiltins,
                                     SymbolId::kUnderNewMemberGetFloat, offset);
    case T_DOUBLE:
      return thread->invokeFunction1(
          SymbolId::kBuiltins, SymbolId::kUnderNewMemberGetDouble, offset);
    case T_LONGLONG:
      return thread->invokeFunction1(SymbolId::kBuiltins,
                                     SymbolId::kUnderNewMemberGetLong, offset);
    case T_ULONGLONG:
      return thread->invokeFunction1(SymbolId::kBuiltins,
                                     SymbolId::kUnderNewMemberGetULong, offset);
    case T_STRING:
      return thread->invokeFunction1(
          SymbolId::kBuiltins, SymbolId::kUnderNewMemberGetString, offset);
    case T_STRING_INPLACE:
      return thread->invokeFunction1(
          SymbolId::kBuiltins, SymbolId::kUnderNewMemberGetString, offset);
    case T_CHAR:
      return thread->invokeFunction1(SymbolId::kBuiltins,
                                     SymbolId::kUnderNewMemberGetChar, offset);
    case T_OBJECT:
      return thread->invokeFunction1(
          SymbolId::kBuiltins, SymbolId::kUnderNewMemberGetPyObject, offset);
    case T_OBJECT_EX:
      return thread->invokeFunction2(SymbolId::kBuiltins,
                                     SymbolId::kUnderNewMemberGetPyObject,
                                     offset, name);
    case T_NONE:
      return thread->invokeFunction1(
          SymbolId::kBuiltins, SymbolId::kUnderNewMemberGetPyObject, offset);
    default:
      return thread->raiseWithFmt(LayoutId::kSystemError,
                                  "bad member name type");
  }
}

static RawObject memberSetter(Thread* thread, PyMemberDef& member) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  if (member.flags & READONLY) {
    Object name(&scope, runtime->newStrFromCStr(member.name));
    Function setter(&scope, thread->invokeFunction1(
                                SymbolId::kBuiltins,
                                SymbolId::kUnderNewMemberSetReadonly, name));
    return *setter;
  }

  Int offset(&scope, runtime->newInt(member.offset));
  switch (member.type) {
    case T_BOOL:
      return thread->invokeFunction1(SymbolId::kBuiltins,
                                     SymbolId::kUnderNewMemberSetBool, offset);
    case T_BYTE: {
      Int num_bytes(&scope, runtime->newInt(1));
      Int min_value(&scope, runtime->newInt(std::numeric_limits<char>::min()));
      Int max_value(&scope, runtime->newInt(std::numeric_limits<char>::max()));
      Str primitive_type(&scope, runtime->newStrFromCStr("char"));
      Function setter(
          &scope, thread->invokeFunction5(
                      SymbolId::kBuiltins, SymbolId::kUnderNewMemberSetIntegral,
                      offset, num_bytes, min_value, max_value, primitive_type));
      return *setter;
    }
    case T_UBYTE: {
      Int num_bytes(&scope, runtime->newInt(1));
      Int min_value(&scope, runtime->newInt(0));
      Int max_value(&scope,
                    runtime->newInt(std::numeric_limits<unsigned char>::max()));
      Str primitive_type(&scope, runtime->newStrFromCStr("unsigned char"));
      Function setter(
          &scope, thread->invokeFunction5(
                      SymbolId::kBuiltins, SymbolId::kUnderNewMemberSetIntegral,
                      offset, num_bytes, min_value, max_value, primitive_type));
      return *setter;
    }
    case T_SHORT: {
      Int num_bytes(&scope, runtime->newInt(2));
      Int min_value(&scope, runtime->newInt(std::numeric_limits<short>::min()));
      Int max_value(&scope, runtime->newInt(std::numeric_limits<short>::max()));
      Str primitive_type(&scope, runtime->newStrFromCStr("short"));
      Function setter(
          &scope, thread->invokeFunction5(
                      SymbolId::kBuiltins, SymbolId::kUnderNewMemberSetIntegral,
                      offset, num_bytes, min_value, max_value, primitive_type));
      return *setter;
    }
    case T_USHORT: {
      Int num_bytes(&scope, runtime->newInt(2));
      Int min_value(&scope, runtime->newInt(0));
      Int max_value(
          &scope, runtime->newInt(std::numeric_limits<unsigned short>::max()));
      Str primitive_type(&scope, runtime->newStrFromCStr("unsigned short"));
      Function setter(
          &scope, thread->invokeFunction5(
                      SymbolId::kBuiltins, SymbolId::kUnderNewMemberSetIntegral,
                      offset, num_bytes, min_value, max_value, primitive_type));
      return *setter;
    }
    case T_INT: {
      Int num_bytes(&scope, runtime->newInt(4));
      Int min_value(&scope, runtime->newInt(std::numeric_limits<int>::min()));
      Int max_value(&scope, runtime->newInt(std::numeric_limits<int>::max()));
      Str primitive_type(&scope, runtime->newStrFromCStr("int"));
      Function setter(
          &scope, thread->invokeFunction5(
                      SymbolId::kBuiltins, SymbolId::kUnderNewMemberSetIntegral,
                      offset, num_bytes, min_value, max_value, primitive_type));
      return *setter;
    }
    case T_UINT: {
      Int num_bytes(&scope, runtime->newInt(4));
      Int min_value(&scope, runtime->newInt(0));
      Int max_value(&scope,
                    runtime->newInt(std::numeric_limits<unsigned int>::max()));
      Str primitive_type(&scope, runtime->newStrFromCStr("unsigned int"));
      Function setter(
          &scope, thread->invokeFunction5(
                      SymbolId::kBuiltins, SymbolId::kUnderNewMemberSetIntegral,
                      offset, num_bytes, min_value, max_value, primitive_type));
      return *setter;
    }
    case T_LONG: {
      Int num_bytes(&scope, runtime->newInt(8));
      Int min_value(&scope, runtime->newInt(std::numeric_limits<long>::min()));
      Int max_value(&scope, runtime->newInt(std::numeric_limits<long>::max()));
      Str primitive_type(&scope, runtime->newStrFromCStr("long"));
      Function setter(
          &scope, thread->invokeFunction5(
                      SymbolId::kBuiltins, SymbolId::kUnderNewMemberSetIntegral,
                      offset, num_bytes, min_value, max_value, primitive_type));
      return *setter;
    }
    case T_ULONG: {
      Int num_bytes(&scope, runtime->newInt(8));
      Int min_value(&scope, runtime->newInt(0));
      Int max_value(&scope,
                    runtime->newInt(std::numeric_limits<unsigned long>::max()));
      Str primitive_type(&scope, runtime->newStrFromCStr("unsigned long"));
      Function setter(
          &scope, thread->invokeFunction5(
                      SymbolId::kBuiltins, SymbolId::kUnderNewMemberSetIntegral,
                      offset, num_bytes, min_value, max_value, primitive_type));
      return *setter;
    }
    case T_PYSSIZET: {
      Int num_bytes(&scope, runtime->newInt(8));
      Int min_value(&scope, runtime->newInt(0));
      Int max_value(&scope,
                    runtime->newInt(std::numeric_limits<Py_ssize_t>::max()));
      Str primitive_type(&scope, runtime->newStrFromCStr("Py_ssize_t"));
      Function setter(
          &scope, thread->invokeFunction5(
                      SymbolId::kBuiltins, SymbolId::kUnderNewMemberSetIntegral,
                      offset, num_bytes, min_value, max_value, primitive_type));
      return *setter;
    }
    case T_FLOAT: {
      return thread->invokeFunction1(SymbolId::kBuiltins,
                                     SymbolId::kUnderNewMemberSetFloat, offset);
    }
    case T_DOUBLE: {
      return thread->invokeFunction1(
          SymbolId::kBuiltins, SymbolId::kUnderNewMemberSetDouble, offset);
    }
    case T_STRING: {
      Object name(&scope, runtime->newStrFromCStr(member.name));
      Function setter(&scope,
                      thread->invokeFunction1(
                          SymbolId::kBuiltins,
                          SymbolId::kUnderNewMemberSetReadonlyStrings, name));
      return *setter;
    }
    case T_STRING_INPLACE: {
      Object name(&scope, runtime->newStrFromCStr(member.name));
      Function setter(&scope,
                      thread->invokeFunction1(
                          SymbolId::kBuiltins,
                          SymbolId::kUnderNewMemberSetReadonlyStrings, name));
      return *setter;
    }
    case T_CHAR: {
      Function setter(&scope, thread->invokeFunction1(
                                  SymbolId::kBuiltins,
                                  SymbolId::kUnderNewMemberSetChar, offset));
      return *setter;
    }
    case T_OBJECT: {
      Function setter(
          &scope, thread->invokeFunction1(SymbolId::kBuiltins,
                                          SymbolId::kUnderNewMemberSetPyObject,
                                          offset));
      return *setter;
    }
    case T_OBJECT_EX: {
      Function setter(
          &scope, thread->invokeFunction1(SymbolId::kBuiltins,
                                          SymbolId::kUnderNewMemberSetPyObject,
                                          offset));
      return *setter;
    }
    case T_LONGLONG: {
      Int num_bytes(&scope, runtime->newInt(8));
      Int min_value(&scope,
                    runtime->newInt(std::numeric_limits<long long>::min()));
      Int max_value(&scope,
                    runtime->newInt(std::numeric_limits<long long>::max()));
      Str primitive_type(&scope, runtime->newStrFromCStr("long long"));
      Function setter(
          &scope, thread->invokeFunction5(
                      SymbolId::kBuiltins, SymbolId::kUnderNewMemberSetIntegral,
                      offset, num_bytes, min_value, max_value, primitive_type));
      return *setter;
    }
    case T_ULONGLONG: {
      Int num_bytes(&scope, runtime->newInt(8));
      Int min_value(&scope, runtime->newInt(0));
      Int max_value(
          &scope,
          runtime->newInt(std::numeric_limits<unsigned long long>::max()));
      Str primitive_type(&scope, runtime->newStrFromCStr("unsigned long long"));
      Function setter(
          &scope, thread->invokeFunction5(
                      SymbolId::kBuiltins, SymbolId::kUnderNewMemberSetIntegral,
                      offset, num_bytes, min_value, max_value, primitive_type));
      return *setter;
    }
    default:
      return thread->raiseWithFmt(LayoutId::kSystemError,
                                  "bad member name type");
  }
}

static RawObject getterWrapper(Thread* thread, Frame* frame, word argc) {
  if (argc != 1) return raiseWrongArgs(thread, argc, 1);
  auto func = getNativeFunc<getter>(thread, frame);
  Arguments args(frame, argc);
  PyObject* self = ApiHandle::borrowedReference(thread, args.get(0));
  return ApiHandle::stealReference(thread, func(self, nullptr));
}

static RawObject setterWrapper(Thread* thread, Frame* frame, word argc) {
  if (argc != 2) return raiseWrongArgs(thread, argc, 2);
  auto func = getNativeFunc<setter>(thread, frame);
  Arguments args(frame, argc);
  PyObject* self = ApiHandle::borrowedReference(thread, args.get(0));
  PyObject* value = ApiHandle::borrowedReference(thread, args.get(1));
  if (func(self, value, nullptr) < 0) return Error::exception();
  return NoneType::object();
}

static RawObject getSetGetter(Thread* thread, const Object& name,
                              PyGetSetDef& def) {
  if (def.get == nullptr) return NoneType::object();
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Function function(&scope, runtime->newFunction());
  function.setName(*name);
  function.setEntry(slotTrampoline);
  function.setEntryKw(slotTrampolineKw);
  function.setEntryEx(slotTrampolineEx);
  if (def.doc != nullptr) {
    Object doc(&scope, runtime->newStrFromCStr(def.doc));
    function.setDoc(*doc);
  }
  Code code(&scope, runtime->newEmptyCode(name));
  code.setCode(runtime->newIntFromCPtr(bit_cast<void*>(&getterWrapper)));
  Tuple consts(&scope, runtime->newTuple(1));
  consts.atPut(0, runtime->newIntFromCPtr(bit_cast<void*>(def.get)));
  code.setConsts(*consts);
  function.setCode(*code);
  return *function;
}

static RawObject getSetSetter(Thread* thread, const Object& name,
                              PyGetSetDef& def) {
  if (def.set == nullptr) return NoneType::object();
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Function function(&scope, runtime->newFunction());
  function.setName(*name);
  function.setEntry(slotTrampoline);
  function.setEntryKw(slotTrampolineKw);
  function.setEntryEx(slotTrampolineEx);
  if (def.doc != nullptr) {
    Object doc(&scope, runtime->newStrFromCStr(def.doc));
    function.setDoc(*doc);
  }
  Code code(&scope, runtime->newEmptyCode(name));
  code.setCode(runtime->newIntFromCPtr(bit_cast<void*>(&setterWrapper)));
  Tuple consts(&scope, runtime->newTuple(1));
  consts.atPut(0, runtime->newIntFromCPtr(bit_cast<void*>(def.set)));
  code.setConsts(*consts);
  function.setCode(*code);
  return *function;
}

RawObject addMembers(Thread* thread, const Type& type) {
  HandleScope scope(thread);
  Object slot_value(&scope, extensionSlot(type, Type::ExtensionSlot::kMembers));
  if (slot_value.isNoneType()) return NoneType::object();
  DCHECK(slot_value.isInt(), "unexpected slot type");
  auto members = bit_cast<PyMemberDef*>(Int::cast(*slot_value).asCPtr());
  Dict dict(&scope, type.dict());
  Object none(&scope, NoneType::object());
  Runtime* runtime = thread->runtime();
  for (word i = 0; members[i].name != nullptr; i++) {
    Object name(&scope, runtime->newStrFromCStr(members[i].name));
    Object getter(&scope, memberGetter(thread, members[i]));
    if (getter.isError()) return *getter;
    Object setter(&scope, memberSetter(thread, members[i]));
    if (setter.isError()) return *setter;
    Object property(&scope, runtime->newProperty(getter, setter, none));
    runtime->typeDictAtPut(thread, dict, name, property);
  }
  return NoneType::object();
}

RawObject addGetSet(Thread* thread, const Type& type) {
  HandleScope scope(thread);
  Object slot_value(&scope, extensionSlot(type, Type::ExtensionSlot::kGetset));
  if (slot_value.isNoneType()) return NoneType::object();
  DCHECK(slot_value.isInt(), "unexpected slot type");
  auto getsets = bit_cast<PyGetSetDef*>(Int::cast(*slot_value).asCPtr());
  Dict dict(&scope, type.dict());
  Object none(&scope, NoneType::object());
  Runtime* runtime = thread->runtime();
  for (word i = 0; getsets[i].name != nullptr; i++) {
    Object name(&scope, runtime->newStrFromCStr(getsets[i].name));
    Object getter(&scope, getSetGetter(thread, name, getsets[i]));
    if (getter.isError()) return *getter;
    Object setter(&scope, getSetSetter(thread, name, getsets[i]));
    if (setter.isError()) return *setter;
    Object property(&scope, runtime->newProperty(getter, setter, none));
    runtime->typeDictAtPut(thread, dict, name, property);
  }
  return NoneType::object();
}

static RawObject addMethod(Thread* thread, const Object& name,
                           PyMethodDef* def) {
  DCHECK(def != nullptr, "methods should not be nullptr");
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Function function(&scope, runtime->newFunction());
  function.setName(*name);
  function.setCode(runtime->newIntFromCPtr(bit_cast<void*>(def->ml_meth)));
  if (def->ml_doc != nullptr) {
    Object doc(&scope, runtime->newStrFromCStr(def->ml_doc));
    function.setDoc(*doc);
  }
  switch (def->ml_flags) {
    case METH_NOARGS:
      function.setEntry(methodTrampolineNoArgs);
      function.setEntryKw(methodTrampolineNoArgsKw);
      function.setEntryEx(methodTrampolineNoArgsEx);
      break;
    case METH_O:
      function.setEntry(methodTrampolineOneArg);
      function.setEntryKw(methodTrampolineOneArgKw);
      function.setEntryEx(methodTrampolineOneArgEx);
      break;
    case METH_VARARGS:
      function.setEntry(methodTrampolineVarArgs);
      function.setEntryKw(methodTrampolineVarArgsKw);
      function.setEntryEx(methodTrampolineVarArgsEx);
      break;
    case METH_VARARGS | METH_KEYWORDS:
      function.setEntry(methodTrampolineKeywords);
      function.setEntryKw(methodTrampolineKeywordsKw);
      function.setEntryEx(methodTrampolineKeywordsEx);
      break;
    case METH_FASTCALL:
      UNIMPLEMENTED("METH_FASTCALL");
      break;
    default:
      UNIMPLEMENTED(
          "Bad call flags in PyCFunction_Call. METH_OLDARGS is no longer "
          "supported!");
  }
  return *function;
}

PY_EXPORT PyObject* PyType_FromSpecWithBases(PyType_Spec* spec,
                                             PyObject* /* bases */) {
  Thread* thread = Thread::current();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  // Create a new type for the PyTypeObject
  Type type(&scope, runtime->newType());
  Dict dict(&scope, runtime->newDict());
  type.setDict(*dict);

  // Set the class name
  const char* class_name = strrchr(spec->name, '.');
  if (class_name == nullptr) {
    class_name = spec->name;
  } else {
    class_name++;
  }
  Object name_obj(&scope, runtime->newStrFromCStr(class_name));
  type.setName(*name_obj);
  Object dict_key(&scope, runtime->symbols()->DunderName());
  runtime->dictAtPutInValueCell(thread, dict, dict_key, name_obj);

  // Compute Mro
  Tuple parents(&scope, runtime->newTuple(0));
  Object mro(&scope, computeMro(thread, type, parents));
  type.setMro(*mro);

  // Initialize instance Layout
  Layout layout(&scope,
                runtime->computeInitialLayout(thread, type, LayoutId::kObject));
  layout.setDescribedType(*type);
  type.setInstanceLayout(*layout);

  // Initialize the extension slots tuple
  Object extension_slots(
      &scope, runtime->newTuple(static_cast<int>(Type::ExtensionSlot::kEnd)));
  type.setExtensionSlots(*extension_slots);

  // Set the type slots
  for (PyType_Slot* slot = spec->slots; slot->slot; slot++) {
    void* slot_ptr = slot->pfunc;
    Object field(&scope, runtime->newIntFromCPtr(slot_ptr));
    Type::ExtensionSlot field_id = slotToTypeSlot(slot->slot);
    if (field_id >= Type::ExtensionSlot::kEnd) {
      thread->raiseWithFmt(LayoutId::kRuntimeError, "invalid slot offset");
      return nullptr;
    }
    setExtensionSlot(type, field_id, *field);
  }

  // Set size
  Object basic_size(&scope, runtime->newInt(spec->basicsize));
  Object item_size(&scope, runtime->newInt(spec->itemsize));
  setExtensionSlot(type, Type::ExtensionSlot::kBasicSize, *basic_size);
  setExtensionSlot(type, Type::ExtensionSlot::kItemSize, *item_size);

  // Set the class flags
  Object flags(&scope, runtime->newInt(spec->flags | Py_TPFLAGS_READY));
  setExtensionSlot(type, Type::ExtensionSlot::kFlags, *flags);

  if (addOperators(thread, type).isError()) return nullptr;

  Object methods_ptr(&scope,
                     extensionSlot(type, Type::ExtensionSlot::kMethods));
  if (!methods_ptr.isNoneType()) {
    PyMethodDef* methods =
        reinterpret_cast<PyMethodDef*>(Int::cast(*methods_ptr).asCPtr());
    for (word i = 0; methods[i].ml_name != nullptr; i++) {
      Object name(&scope, runtime->newStrFromCStr(methods[i].ml_name));
      Object function(&scope, addMethod(thread, name, &methods[i]));
      runtime->dictAtPutInValueCell(thread, dict, name, function);
    }
  }

  if (addMembers(thread, type).isError()) return nullptr;

  if (addGetSet(thread, type).isError()) return nullptr;

  return ApiHandle::newReference(thread, *type);
}

PY_EXPORT PyObject* PyType_GenericAlloc(PyTypeObject* type_obj,
                                        Py_ssize_t nitems) {
  DCHECK(ApiHandle::isManaged(reinterpret_cast<PyObject*>(type_obj)),
         "Type is unmanaged. Please initialize using PyType_FromSpec");
  HandleScope scope;
  Type type(&scope,
            ApiHandle::fromPyObject(reinterpret_cast<PyObject*>(type_obj))
                ->asObject());
  DCHECK(!type.isBuiltin(),
         "Type is unmanaged. Please initialize using PyType_FromSpec");
  DCHECK(!type.extensionSlots().isNoneType(),
         "GenericAlloc from types initialized through Python code");
  Int basic_size(&scope, extensionSlot(type, Type::ExtensionSlot::kBasicSize));
  Int item_size(&scope, extensionSlot(type, Type::ExtensionSlot::kItemSize));
  Py_ssize_t size = Utils::roundUp(
      nitems * item_size.asWord() + basic_size.asWord(), kWordSize);

  PyObject* pyobj = static_cast<PyObject*>(PyObject_Calloc(1, size));
  if (pyobj == nullptr) return nullptr;
  pyobj->ob_refcnt = 1;
  pyobj->ob_type = type_obj;
  if (item_size.asWord() != 0) {
    reinterpret_cast<PyVarObject*>(pyobj)->ob_size = nitems;
  }
  return pyobj;
}

PY_EXPORT Py_ssize_t _PyObject_SIZE_Func(PyObject* obj) {
  HandleScope scope;
  Type type(&scope, ApiHandle::fromPyObject(obj)->asObject());
  Int basic_size(&scope, extensionSlot(type, Type::ExtensionSlot::kBasicSize));
  return basic_size.asWord();
}

PY_EXPORT Py_ssize_t _PyObject_VAR_SIZE_Func(PyObject* obj, Py_ssize_t nitems) {
  HandleScope scope;
  Type type(&scope, ApiHandle::fromPyObject(obj)->asObject());
  Int basic_size(&scope, extensionSlot(type, Type::ExtensionSlot::kBasicSize));
  Int item_size(&scope, extensionSlot(type, Type::ExtensionSlot::kItemSize));
  return Utils::roundUp(nitems * item_size.asWord() + basic_size.asWord(),
                        kWordSize);
}

PY_EXPORT unsigned int PyType_ClearCache() {
  UNIMPLEMENTED("PyType_ClearCache");
}

PY_EXPORT PyObject* PyType_GenericNew(PyTypeObject* type, PyObject*,
                                      PyObject*) {
  auto alloc_func =
      reinterpret_cast<allocfunc>(PyType_GetSlot(type, Py_tp_alloc));
  return alloc_func(type, 0);
}

PY_EXPORT int PyType_IsSubtype(PyTypeObject* a, PyTypeObject* b) {
  if (a == b) return 1;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Type a_obj(
      &scope,
      ApiHandle::fromPyObject(reinterpret_cast<PyObject*>(a))->asObject());
  Type b_obj(
      &scope,
      ApiHandle::fromPyObject(reinterpret_cast<PyObject*>(b))->asObject());
  return thread->runtime()->isSubclass(a_obj, b_obj) ? 1 : 0;
}

PY_EXPORT void PyType_Modified(PyTypeObject* /* e */) {
  UNIMPLEMENTED("PyType_Modified");
}

PY_EXPORT PyObject* _PyObject_LookupSpecial(PyObject* /* f */,
                                            _Py_Identifier* /* d */) {
  UNIMPLEMENTED("_PyObject_LookupSpecial");
}

}  // namespace python

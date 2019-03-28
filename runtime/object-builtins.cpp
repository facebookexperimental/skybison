#include "object-builtins.h"

#include <cinttypes>

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"
#include "trampolines-inl.h"

namespace python {

// clang-format off
const BuiltinMethod ObjectBuiltins::kBuiltinMethods[] = {
    {SymbolId::kDunderHash, dunderHash},
    {SymbolId::kDunderInit, dunderInit},
    {SymbolId::kDunderNew, dunderNew},
    {SymbolId::kDunderSizeof, dunderSizeof},
    // no sentinel needed because the iteration below is manual
};
// clang-format on

void ObjectBuiltins::initialize(Runtime* runtime) {
  HandleScope scope;

  Layout layout(&scope, runtime->newLayout());
  layout.setId(LayoutId::kObject);
  Type object_type(&scope, runtime->newType());
  layout.setDescribedType(*object_type);
  object_type.setName(runtime->symbols()->ObjectTypename());
  Tuple mro(&scope, runtime->newTuple(1));
  mro.atPut(0, *object_type);
  object_type.setMro(*mro);
  object_type.setInstanceLayout(*layout);
  runtime->layoutAtPut(LayoutId::kObject, *layout);

  for (uword i = 0; i < ARRAYSIZE(kBuiltinMethods); i++) {
    runtime->typeAddBuiltinFunction(object_type, kBuiltinMethods[i].name,
                                    kBuiltinMethods[i].address);
  }
}

RawObject ObjectBuiltins::dunderHash(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  return thread->runtime()->hash(args.get(0));
}

RawObject ObjectBuiltins::dunderInit(Thread* thread, Frame* frame, word nargs) {
  // Too many arguments were given. Determine if the __new__ was not overwritten
  // or the __init__ was to throw a TypeError.
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object self(&scope, args.get(0));
  Tuple starargs(&scope, args.get(1));
  Dict kwargs(&scope, args.get(2));
  if (starargs.length() == 0 && kwargs.numItems() == 0) {
    // object.__init__ doesn't do anything except throw a TypeError if the
    // wrong number of arguments are given. It only throws if __new__ is not
    // overloaded or __init__ was overloaded, else it allows the excess
    // arguments.
    return NoneType::object();
  }
  Type type(&scope, runtime->typeOf(*self));
  if (!runtime->isMethodOverloaded(thread, type, SymbolId::kDunderNew) ||
      runtime->isMethodOverloaded(thread, type, SymbolId::kDunderInit)) {
    // Throw a TypeError if extra arguments were passed, and __new__ was not
    // overwritten by self, or __init__ was overloaded by self.
    return thread->raiseTypeErrorWithCStr(
        "object.__init__() takes no parameters");
  }
  // Else it's alright to have extra arguments.
  return NoneType::object();
}

RawObject ObjectBuiltins::dunderNew(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  if (nargs < 1) {
    return thread->raiseTypeErrorWithCStr(
        "object.__new__() takes no arguments");
  }
  HandleScope scope(thread);
  Type type(&scope, args.get(0));
  Layout layout(&scope, type.instanceLayout());
  return thread->runtime()->newInstance(layout);
}

RawObject ObjectBuiltins::dunderNewKw(Thread* thread, Frame* frame,
                                      word nargs) {
  // This should really raise an error if __init__ is not overridden (see
  // https://hlrz.com/source/xref/cpython-3.6/Objects/typeobject.c#3428)
  // However, object.__new__ should also do that as well. For now, just forward
  // to __new__.
  KwArguments args(frame, nargs);
  return dunderNew(thread, frame, nargs - args.numKeywords() - 1);
}

RawObject ObjectBuiltins::dunderSizeof(Thread* thread, Frame* frame,
                                       word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object obj(&scope, args.get(0));
  if (obj.isHeapObject()) {
    HeapObject heap_obj(&scope, *obj);
    return SmallInt::fromWord(heap_obj.size());
  }
  return SmallInt::fromWord(kPointerSize);
}

const BuiltinMethod NoneBuiltins::kBuiltinMethods[] = {
    {SymbolId::kDunderNew, dunderNew},
    {SymbolId::kDunderRepr, dunderRepr},
    {SymbolId::kSentinelId, nullptr},
};

RawObject NoneBuiltins::dunderNew(Thread*, Frame*, word) {
  return NoneType::object();
}

RawObject NoneBuiltins::dunderRepr(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  if (!args.get(0).isNoneType()) {
    return thread->raiseTypeErrorWithCStr(
        "__repr__ expects None as first argument");
  }
  return thread->runtime()->symbols()->None();
}

}  // namespace python

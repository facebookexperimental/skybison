#include "range-builtins.h"

#include "frame.h"
#include "globals.h"
#include "runtime.h"
#include "trampolines-inl.h"

namespace python {

void RangeBuiltins::initialize(Runtime* runtime) {
  HandleScope scope;
  Handle<Type> range(
      &scope, runtime->addEmptyBuiltinClass(SymbolId::kRange, LayoutId::kRange,
                                            LayoutId::kObject));
  runtime->classAddBuiltinFunction(range, SymbolId::kDunderIter,
                                   nativeTrampoline<dunderIter>);
}

Object* RangeBuiltins::dunderIter(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 1) {
    return thread->raiseTypeErrorWithCStr("__iter__() takes no arguments");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Handle<Object> self(&scope, args.get(0));
  if (!self->isRange()) {
    return thread->raiseTypeErrorWithCStr(
        "__getitem__() must be called with a range instance as the first "
        "argument");
  }
  return thread->runtime()->newRangeIterator(self);
}

const BuiltinMethod RangeIteratorBuiltins::kMethods[] = {
    {SymbolId::kDunderIter, nativeTrampoline<dunderIter>},
    {SymbolId::kDunderNext, nativeTrampoline<dunderNext>},
    {SymbolId::kDunderLengthHint, nativeTrampoline<dunderLengthHint>}};

void RangeIteratorBuiltins::initialize(Runtime* runtime) {
  HandleScope scope;
  Handle<Type> range_iter(
      &scope, runtime->addBuiltinClass(SymbolId::kRangeIterator,
                                       LayoutId::kRangeIterator,
                                       LayoutId::kObject, kMethods));
}

Object* RangeIteratorBuiltins::dunderIter(Thread* thread, Frame* frame,
                                          word nargs) {
  if (nargs != 1) {
    return thread->raiseTypeErrorWithCStr("__iter__() takes no arguments");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Handle<Object> self(&scope, args.get(0));
  if (!self->isRangeIterator()) {
    return thread->raiseTypeErrorWithCStr(
        "__iter__() must be called with a range iterator instance as the first "
        "argument");
  }
  return *self;
}

Object* RangeIteratorBuiltins::dunderNext(Thread* thread, Frame* frame,
                                          word nargs) {
  if (nargs != 1) {
    return thread->raiseTypeErrorWithCStr("__next__() takes no arguments");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Handle<Object> self(&scope, args.get(0));
  if (!self->isRangeIterator()) {
    return thread->raiseTypeErrorWithCStr(
        "__next__() must be called with a range iterator instance as the first "
        "argument");
  }
  Handle<Object> value(&scope, RangeIterator::cast(*self)->next());
  if (value->isError()) {
    return thread->raiseStopIteration(None::object());
  }
  return *value;
}

Object* RangeIteratorBuiltins::dunderLengthHint(Thread* thread, Frame* frame,
                                                word nargs) {
  if (nargs != 1) {
    return thread->raiseTypeErrorWithCStr(
        "__length_hint__() takes no arguments");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Handle<Object> self(&scope, args.get(0));
  if (!self->isRangeIterator()) {
    return thread->raiseTypeErrorWithCStr(
        "__length_hint__() must be called with a range iterator instance as "
        "the first argument");
  }
  Handle<RangeIterator> range_iterator(&scope, *self);
  return SmallInt::fromWord(range_iterator->pendingLength());
}

}  // namespace python

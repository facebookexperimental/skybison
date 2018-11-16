#include "list-builtins.h"

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"
#include "trampolines-inl.h"

namespace python {

const BuiltinAttribute ListBuiltins::kAttributes[] = {
    {SymbolId::kItems, RawList::kItemsOffset},
    {SymbolId::kAllocated, RawList::kAllocatedOffset},
};

const BuiltinMethod ListBuiltins::kMethods[] = {
    {SymbolId::kAppend, nativeTrampoline<append>},
    {SymbolId::kDunderAdd, nativeTrampoline<dunderAdd>},
    {SymbolId::kDunderDelItem, nativeTrampoline<dunderDelItem>},
    {SymbolId::kDunderGetItem, nativeTrampoline<dunderGetItem>},
    {SymbolId::kDunderIter, nativeTrampoline<dunderIter>},
    {SymbolId::kDunderLen, nativeTrampoline<dunderLen>},
    {SymbolId::kDunderMul, nativeTrampoline<dunderMul>},
    {SymbolId::kDunderNew, nativeTrampoline<dunderNew>},
    {SymbolId::kDunderSetItem, nativeTrampoline<dunderSetItem>},
    {SymbolId::kExtend, nativeTrampoline<extend>},
    {SymbolId::kInsert, nativeTrampoline<insert>},
    {SymbolId::kPop, nativeTrampoline<pop>},
    {SymbolId::kRemove, nativeTrampoline<remove>}};

void ListBuiltins::initialize(Runtime* runtime) {
  HandleScope scope;

  Type list(&scope,
            runtime->addBuiltinClass(SymbolId::kList, LayoutId::kList,
                                     LayoutId::kObject, kAttributes, kMethods));
  list->setFlag(Type::Flag::kListSubclass);
}

RawObject ListBuiltins::dunderNew(Thread* thread, Frame* frame, word nargs) {
  if (nargs < 1) {
    return thread->raiseTypeErrorWithCStr("not enough arguments");
  }
  Arguments args(frame, nargs);
  if (!args.get(0)->isType()) {
    return thread->raiseTypeErrorWithCStr("not a type object");
  }
  HandleScope scope(thread);
  Type type(&scope, args.get(0));
  if (!type->hasFlag(Type::Flag::kListSubclass)) {
    return thread->raiseTypeErrorWithCStr("not a subtype of list");
  }
  Layout layout(&scope, type->instanceLayout());
  List result(&scope, thread->runtime()->newInstance(layout));
  result->setNumItems(0);
  result->setItems(thread->runtime()->newObjectArray(0));
  return *result;
}

RawObject ListBuiltins::dunderAdd(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->raiseTypeErrorWithCStr("expected 1 argument");
  }

  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfList(*self)) {
    return thread->raiseTypeErrorWithCStr(
        "__add__() must be called with list instance as first argument");
  }

  Object other(&scope, args.get(1));
  if (other->isList()) {
    word new_capacity =
        RawList::cast(*self)->numItems() + RawList::cast(*other)->numItems();
    List new_list(&scope, runtime->newList());
    runtime->listEnsureCapacity(new_list, new_capacity);
    new_list = runtime->listExtend(thread, new_list, self);
    if (!new_list->isError()) {
      new_list = runtime->listExtend(thread, new_list, other);
    }
    return *new_list;
  }
  return thread->raiseTypeErrorWithCStr("can only concatenate list to list");
}

RawObject ListBuiltins::append(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->raiseTypeErrorWithCStr(
        "append() takes exactly one argument");
  }
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfList(*self)) {
    return thread->raiseTypeErrorWithCStr(
        "append() only support list or its subclasses");
  }
  List list(&scope, *self);
  Object value(&scope, args.get(1));
  thread->runtime()->listAdd(list, value);
  return NoneType::object();
}

RawObject ListBuiltins::extend(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->raiseTypeErrorWithCStr(
        "extend() takes exactly one argument");
  }
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfList(*self)) {
    return thread->raiseTypeErrorWithCStr(
        "extend() only support list or its subclasses");
  }
  List list(&scope, *self);
  Object value(&scope, args.get(1));
  list = thread->runtime()->listExtend(thread, list, value);
  if (list->isError()) {
    return *list;
  }
  return NoneType::object();
}

RawObject ListBuiltins::dunderLen(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 1) {
    return thread->raiseTypeErrorWithCStr("__len__() takes no arguments");
  }
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfList(*self)) {
    return thread->raiseTypeErrorWithCStr(
        "__len__() only support list or its subclasses");
  }
  List list(&scope, *self);
  return SmallInt::fromWord(list->numItems());
}

RawObject ListBuiltins::insert(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 3) {
    return thread->raiseTypeErrorWithCStr(
        "insert() takes exactly two arguments");
  }
  Arguments args(frame, nargs);
  if (!args.get(1)->isInt()) {
    return thread->raiseTypeErrorWithCStr(
        "index object cannot be interpreted as an integer");
  }

  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfList(*self)) {
    return thread->raiseTypeErrorWithCStr(
        "descriptor 'insert' requires a 'list' object");
  }
  List list(&scope, *self);
  word index = RawSmallInt::cast(args.get(1))->value();
  Object value(&scope, args.get(2));
  thread->runtime()->listInsert(list, value, index);
  return NoneType::object();
}

RawObject ListBuiltins::dunderMul(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->raiseTypeErrorWithCStr("expected 1 argument");
  }
  Arguments args(frame, nargs);
  RawObject other = args.get(1);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfList(*self)) {
    return thread->raiseTypeErrorWithCStr(
        "__mul__() must be called with list instance as first argument");
  }
  if (other->isSmallInt()) {
    word ntimes = RawSmallInt::cast(other)->value();
    List list(&scope, *self);
    return thread->runtime()->listReplicate(thread, list, ntimes);
  }
  return thread->raiseTypeErrorWithCStr("can't multiply list by non-int");
}

RawObject ListBuiltins::pop(Thread* thread, Frame* frame, word nargs) {
  if (nargs > 2) {
    return thread->raiseTypeErrorWithCStr("pop() takes at most 1 argument");
  }
  Arguments args(frame, nargs);
  if (nargs == 2 && !args.get(1)->isSmallInt()) {
    return thread->raiseTypeErrorWithCStr(
        "index object cannot be interpreted as an integer");
  }

  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfList(*self)) {
    return thread->raiseTypeErrorWithCStr(
        "descriptor 'pop' requires a 'list' object");
  }
  List list(&scope, *self);
  word index = list->numItems() - 1;
  if (nargs == 2) {
    word last_index = index;
    index = RawSmallInt::cast(args.get(1))->value();
    index = index < 0 ? last_index + index : index;
    // Pop out of bounds
    if (index > last_index) {
      // TODO(T27365047): Throw an IndexError exception
      UNIMPLEMENTED("Throw an RawIndexError for an out of range list index.");
    }
  }
  // Pop empty, or negative out of bounds
  if (index < 0) {
    // TODO(T27365047): Throw an IndexError exception
    UNIMPLEMENTED("Throw an RawIndexError for an out of range list index.");
  }

  return thread->runtime()->listPop(list, index);
}

RawObject ListBuiltins::remove(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->raiseTypeErrorWithCStr(
        "remove() takes exactly one argument");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  Object value(&scope, args.get(1));
  if (!thread->runtime()->isInstanceOfList(*self)) {
    return thread->raiseTypeErrorWithCStr(
        "descriptor 'remove' requires a 'list' object");
  }
  List list(&scope, *self);
  for (word i = 0; i < list->numItems(); i++) {
    Object item(&scope, list->at(i));
    if (RawBool::cast(Interpreter::compareOperation(thread, frame,
                                                    CompareOp::EQ, item, value))
            ->value()) {
      thread->runtime()->listPop(list, i);
      return NoneType::object();
    }
  }
  return thread->raiseValueErrorWithCStr("list.remove(x) x not in list");
}

RawObject ListBuiltins::slice(Thread* thread, RawList list, RawSlice slice) {
  word start, stop, step;
  slice->unpack(&start, &stop, &step);
  word length = Slice::adjustIndices(list->numItems(), &start, &stop, step);

  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  ObjectArray items(&scope, runtime->newObjectArray(length));
  for (word i = 0, index = start; i < length; i++, index += step) {
    items->atPut(i, list->at(index));
  }

  List result(&scope, runtime->newList());
  result->setItems(*items);
  result->setNumItems(items->length());
  return *result;
}

RawObject ListBuiltins::dunderGetItem(Thread* thread, Frame* frame,
                                      word nargs) {
  if (nargs != 2) {
    return thread->raiseTypeErrorWithCStr("expected 1 argument");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));

  if (!thread->runtime()->isInstanceOfList(*self)) {
    return thread->raiseTypeErrorWithCStr(
        "__getitem__() must be called with a list instance as the first "
        "argument");
  }

  List list(&scope, *self);
  RawObject index = args.get(1);
  if (index->isSmallInt()) {
    word idx = RawSmallInt::cast(index)->value();
    if (idx < 0) {
      idx = list->numItems() - idx;
    }
    if (idx < 0 || idx >= list->numItems()) {
      return thread->raiseIndexErrorWithCStr("list index out of range");
    }
    return list->at(idx);
  } else if (index->isSlice()) {
    Slice list_slice(&scope, RawSlice::cast(index));
    return slice(thread, *list, *list_slice);
  } else {
    return thread->raiseTypeErrorWithCStr(
        "list indices must be integers or slices");
  }
}

RawObject ListBuiltins::dunderIter(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 1) {
    return thread->raiseTypeErrorWithCStr("__iter__() takes no arguments");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfList(*self)) {
    return thread->raiseTypeErrorWithCStr(
        "__iter__() must be called with a list instance as the first argument");
  }
  return thread->runtime()->newListIterator(self);
}

const BuiltinMethod ListIteratorBuiltins::kMethods[] = {
    {SymbolId::kDunderIter, nativeTrampoline<dunderIter>},
    {SymbolId::kDunderNext, nativeTrampoline<dunderNext>},
    {SymbolId::kDunderLengthHint, nativeTrampoline<dunderLengthHint>}};

void ListIteratorBuiltins::initialize(Runtime* runtime) {
  HandleScope scope;
  Type list_iter(&scope, runtime->addBuiltinClass(SymbolId::kListIterator,
                                                  LayoutId::kListIterator,
                                                  LayoutId::kObject, kMethods));
}

RawObject ListIteratorBuiltins::dunderIter(Thread* thread, Frame* frame,
                                           word nargs) {
  if (nargs != 1) {
    return thread->raiseTypeErrorWithCStr("__iter__() takes no arguments");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self->isListIterator()) {
    return thread->raiseTypeErrorWithCStr(
        "__iter__() must be called with a list iterator instance as the first "
        "argument");
  }
  return *self;
}

RawObject ListIteratorBuiltins::dunderNext(Thread* thread, Frame* frame,
                                           word nargs) {
  if (nargs != 1) {
    return thread->raiseTypeErrorWithCStr("__next__() takes no arguments");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self->isListIterator()) {
    return thread->raiseTypeErrorWithCStr(
        "__next__() must be called with a list iterator instance as the first "
        "argument");
  }
  Object value(&scope, RawListIterator::cast(*self)->next());
  if (value->isError()) {
    return thread->raiseStopIteration(NoneType::object());
  }
  return *value;
}

RawObject ListIteratorBuiltins::dunderLengthHint(Thread* thread, Frame* frame,
                                                 word nargs) {
  if (nargs != 1) {
    return thread->raiseTypeErrorWithCStr(
        "__length_hint__() takes no arguments");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self->isListIterator()) {
    return thread->raiseTypeErrorWithCStr(
        "__length_hint__() must be called with a list iterator instance as the "
        "first argument");
  }
  ListIterator list_iterator(&scope, *self);
  List list(&scope, list_iterator->list());
  return SmallInt::fromWord(list->numItems() - list_iterator->index());
}

RawObject ListBuiltins::dunderSetItem(Thread* thread, Frame* frame,
                                      word nargs) {
  if (nargs != 3) {
    return thread->raiseTypeErrorWithCStr("expected 3 arguments");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));

  if (!thread->runtime()->isInstanceOfList(*self)) {
    return thread->raiseTypeErrorWithCStr(
        "__setitem__() must be called with a list instance as the first "
        "argument");
  }

  List list(&scope, *self);
  RawObject index = args.get(1);
  if (index->isSmallInt()) {
    word idx = RawSmallInt::cast(index)->value();
    if (idx < 0) {
      idx = list->numItems() + idx;
    }
    if (idx < 0 || idx >= list->numItems()) {
      return thread->raiseIndexErrorWithCStr(
          "list assignment index out of range");
    }
    Object value(&scope, args.get(2));
    list->atPut(idx, *value);
    return NoneType::object();
  }
  // TODO(T31826482): Add support for slices
  return thread->raiseTypeErrorWithCStr(
      "list indices must be integers or slices");
}

RawObject ListBuiltins::dunderDelItem(Thread* thread, Frame* frame,
                                      word nargs) {
  if (nargs != 2) {
    return thread->raiseTypeErrorWithCStr("expected 2 arguments");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));

  if (!thread->runtime()->isInstanceOfList(*self)) {
    return thread->raiseTypeErrorWithCStr(
        "__delitem__() must be called with a list instance as the first "
        "argument");
  }

  List list(&scope, *self);
  RawObject index = args.get(1);
  if (index->isSmallInt()) {
    word idx = RawSmallInt::cast(index)->value();
    idx = idx < 0 ? list->numItems() + idx : idx;
    if (idx < 0 || idx >= list->numItems()) {
      return thread->raiseIndexErrorWithCStr(
          "list assignment index out of range");
    }
    return thread->runtime()->listPop(list, idx);
  }
  // TODO(T31826482): Add support for slices
  return thread->raiseTypeErrorWithCStr(
      "list indices must be integers or slices");
}

}  // namespace python

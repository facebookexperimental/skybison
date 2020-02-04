#pragma once

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace py {

// Extends a list from an exact tuple or exact list. Modifies dst in-place.
// Returns either the None or an Error object if allocation failed.
RawObject listExtend(Thread* thread, const List& dst, const Object& iterable);

// Inserts an element to the specified index of the list.
// When index >= len(list) it is equivalent to appending to the list.
void listInsert(Thread* thread, const List& list, const Object& value,
                word index);

// Removes and returns an element from the specified list index.
// Expects index to be within [0, len(list)]
RawObject listPop(Thread* thread, const List& list, word index);

// Return a new list that is composed of list repeated ntimes
RawObject listReplicate(Thread* thread, const List& list, word ntimes);

void listReverse(Thread* thread, const List& list);

// Returns a new list by slicing the given list
RawObject listSlice(Thread* thread, const List& list, word start, word stop,
                    word step);

// Sort a list in place.
// Returns None when there has been no error, or throws a TypeError and returns
// Error otherwise.
RawObject listSort(Thread* thread, const List& list);

// Return the next item from the iterator, or Error if there are no items left.
RawObject listIteratorNext(Thread* thread, const ListIterator& iter);

class ListBuiltins : public Builtins<ListBuiltins, ID(list), LayoutId::kList> {
 public:
  static const BuiltinAttribute kAttributes[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(ListBuiltins);
};

class ListIteratorBuiltins
    : public Builtins<ListIteratorBuiltins, ID(list_iterator),
                      LayoutId::kListIterator> {
 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(ListIteratorBuiltins);
};

}  // namespace py

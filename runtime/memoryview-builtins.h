/* Copyright (c) Facebook, Inc. and its affiliates. (http://www.facebook.com) */
#pragma once

#include "globals.h"
#include "runtime.h"

namespace py {

RawObject memoryviewGetitem(Thread* thread, const MemoryView& view, word index);
RawObject memoryviewGetslice(Thread* thread, const MemoryView& view, word start,
                             word stop, word step);
word memoryviewItemsize(Thread* thread, const MemoryView& view);
RawObject memoryviewSetitem(Thread* thread, const MemoryView& view, word index,
                            const Object& value);
RawObject memoryviewSetslice(Thread* thread, const MemoryView& view, word start,
                             word stop, word step, word slice_len,
                             const Object& value);

void initializeMemoryViewType(Thread* thread);

}  // namespace py

/* Copyright (c) Facebook, Inc. and its affiliates. (http://www.facebook.com) */
#pragma once

#include "cpython-types.h"

#include "api-handle-dict.h"
#include "api-handle.h"
#include "capi.h"
#include "runtime.h"
#include "vector.h"

namespace py {

struct ListEntry;

struct CAPIState {
  // Some API functions promise to cache their return value and return the same
  // value for repeated invocations on a specific PyObject. Those value are
  // cached here.
  ApiHandleDict caches;

  // A linked list of freed handles.
  // The last node is the frontier of allocated handles.
  FreeListNode* free_handles;

  // The raw memory used to allocated handles.
  byte* handle_buffer;
  word handle_buffer_size;

  // C-API object handles
  ApiHandleDict handles;

  Vector<PyObject*> modules;

  ListEntry* extension_objects;
  word num_extension_objects;
};

static_assert(sizeof(CAPIState) < kCAPIStateSize, "kCAPIStateSize too small");

inline CAPIState* capiState(Runtime* runtime) {
  return reinterpret_cast<CAPIState*>(runtime->capiStateData());
}

inline ApiHandleDict* capiCaches(Runtime* runtime) {
  return &capiState(runtime)->caches;
}

inline FreeListNode** capiFreeHandles(Runtime* runtime) {
  return &capiState(runtime)->free_handles;
}

inline ApiHandleDict* capiHandles(Runtime* runtime) {
  return &capiState(runtime)->handles;
}

inline Vector<PyObject*>* capiModules(Runtime* runtime) {
  return &capiState(runtime)->modules;
}

}  // namespace py

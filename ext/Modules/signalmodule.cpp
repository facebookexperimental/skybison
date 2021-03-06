// Copyright (c) Facebook, Inc. and its affiliates. (http://www.facebook.com)
#include <csignal>

#include "cpython-func.h"

#include "runtime.h"
#include "under-imp-module.h"

namespace py {

PY_EXPORT void _PySignal_AfterFork() {
  // TODO(T39596544): do nothing until we have a GIL.
}

PY_EXPORT int PyErr_CheckSignals() {
  Thread* thread = Thread::current();
  if (thread->runtime()->handlePendingSignals(thread).isErrorException()) {
    return -1;
  }
  return 0;
}

PY_EXPORT void PyErr_SetInterrupt() {
  Thread* thread = Thread::current();
  thread->runtime()->setPendingSignal(thread, SIGINT);
}

PY_EXPORT void PyOS_InitInterrupts() { UNIMPLEMENTED("PyOS_InitInterrupts"); }

PY_EXPORT int PyOS_InterruptOccurred(void) {
  UNIMPLEMENTED("PyOS_InterruptOccurred");
}

PY_EXPORT int _PyOS_InterruptOccurred(PyThreadState*) {
  UNIMPLEMENTED("_PyOS_InterruptOccurred");
}

}  // namespace py

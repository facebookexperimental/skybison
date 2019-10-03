#include "capi-handles.h"
#include "runtime.h"

namespace python {

PY_EXPORT PyObject* PySeqIter_New(PyObject* seq) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object seq_obj(&scope, ApiHandle::fromPyObject(seq)->asObject());
  Runtime* runtime = thread->runtime();
  if (!runtime->isSequence(thread, seq_obj)) {
    thread->raiseBadInternalCall();
    return nullptr;
  }
  return ApiHandle::newReference(thread, runtime->newSeqIterator(seq_obj));
}

PY_EXPORT PyObject* PyCallIter_New(PyObject* pycallable, PyObject* pysentinel) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object callable(&scope, ApiHandle::fromPyObject(pycallable)->asObject());
  Object sentinel(&scope, ApiHandle::fromPyObject(pysentinel)->asObject());
  Object result(&scope, thread->invokeFunction2(SymbolId::kBuiltins,
                                                SymbolId::kCallableIterator,
                                                callable, sentinel));
  if (result.isErrorException()) return nullptr;
  return ApiHandle::newReference(thread, *result);
}

PY_EXPORT int PyIter_Check_Func(PyObject* iter) {
  return ApiHandle::fromPyObject(iter)->asObject().isSeqIterator();
}

}  // namespace python

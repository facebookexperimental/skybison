#include "runtime.h"

namespace python {

PY_EXPORT int PyByteArray_CheckExact_Func(PyObject* pyobj) {
  return ApiHandle::fromPyObject(pyobj)->asObject()->isByteArray();
}

PY_EXPORT int PyByteArray_Check_Func(PyObject* pyobj) {
  return Thread::currentThread()->runtime()->isInstanceOfByteArray(
      ApiHandle::fromPyObject(pyobj)->asObject());
}

PY_EXPORT PyObject* PyByteArray_FromStringAndSize(const char* str,
                                                  Py_ssize_t size) {
  Thread* thread = Thread::currentThread();
  if (size < 0) {
    thread->raiseSystemErrorWithCStr(
        "Negative size passed to PyByteArray_FromStringAndSize");
    return nullptr;
  }
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  ByteArray result(&scope, runtime->newByteArray());
  if (size > 0) {
    word capacity = static_cast<word>(size);
    if (str == nullptr) {
      result->setBytes(runtime->newBytes(capacity, 0));
    } else {
      View<byte> view(reinterpret_cast<const byte*>(str), capacity);
      result->setBytes(runtime->newBytesWithAll(view));
    }
    result->setNumBytes(capacity);
  }
  return ApiHandle::newReference(thread, *result);
}

PY_EXPORT int PyByteArray_Resize(PyObject* /* f */, Py_ssize_t /* e */) {
  UNIMPLEMENTED("PyByteArray_Resize");
}

PY_EXPORT char* PyByteArray_AsString(PyObject* /* f */) {
  UNIMPLEMENTED("PyByteArray_AsString");
}

PY_EXPORT PyObject* PyByteArray_Concat(PyObject* /* a */, PyObject* /* b */) {
  UNIMPLEMENTED("PyByteArray_Concat");
}

PY_EXPORT PyObject* PyByteArray_FromObject(PyObject* /* t */) {
  UNIMPLEMENTED("PyByteArray_FromObject");
}

PY_EXPORT Py_ssize_t PyByteArray_Size(PyObject* /* f */) {
  UNIMPLEMENTED("PyByteArray_Size");
}

}  // namespace python

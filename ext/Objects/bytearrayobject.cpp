#include "bytearray-builtins.h"
#include "runtime.h"

namespace python {

PY_EXPORT char* PyByteArray_AsString(PyObject* pyobj) {
  DCHECK(pyobj != nullptr, "null argument to PyByteArray_AsString");
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  ApiHandle* handle = ApiHandle::fromPyObject(pyobj);
  Object obj(&scope, handle->asObject());
  Runtime* runtime = thread->runtime();
  DCHECK(runtime->isInstanceOfByteArray(*obj),
         "argument to PyByteArray_AsString is not a bytearray");
  if (void* cache = handle->cache()) std::free(cache);
  ByteArray array(&scope, *obj);
  word len = array.numItems();
  auto buffer = static_cast<byte*>(std::malloc(len + 1));
  RawBytes::cast(array.bytes()).copyTo(buffer, len);
  buffer[len] = '\0';
  handle->setCache(buffer);
  return reinterpret_cast<char*>(buffer);
}

PY_EXPORT int PyByteArray_CheckExact_Func(PyObject* pyobj) {
  return ApiHandle::fromPyObject(pyobj)->asObject()->isByteArray();
}

PY_EXPORT int PyByteArray_Check_Func(PyObject* pyobj) {
  return Thread::currentThread()->runtime()->isInstanceOfByteArray(
      ApiHandle::fromPyObject(pyobj)->asObject());
}

PY_EXPORT PyObject* PyByteArray_Concat(PyObject* a, PyObject* b) {
  DCHECK(a != nullptr, "null argument to PyByteArray_Concat");
  DCHECK(b != nullptr, "null argument to PyByteArray_Concat");
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Object left(&scope, ApiHandle::fromPyObject(a)->asObject());
  Object right(&scope, ApiHandle::fromPyObject(b)->asObject());
  Runtime* runtime = thread->runtime();
  bool valid_left = runtime->isInstanceOfByteArray(*left) ||
                    runtime->isInstanceOfBytes(*left);
  bool valid_right = runtime->isInstanceOfByteArray(*right) ||
                     runtime->isInstanceOfBytes(*right);
  if (!valid_left || !valid_right) {
    thread->raiseTypeErrorWithCStr("can only concatenate bytearray or bytes");
    return nullptr;
  }
  Object result(&scope, runtime->newByteArray());
  result = thread->invokeFunction2(SymbolId::kOperator, SymbolId::kIconcat,
                                   result, left);
  if (result.isError()) return nullptr;
  result = thread->invokeFunction2(SymbolId::kOperator, SymbolId::kIconcat,
                                   result, right);
  if (result.isError()) return nullptr;
  return ApiHandle::newReference(thread, *result);
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
      result.setBytes(runtime->newBytes(capacity, 0));
    } else {
      View<byte> view(reinterpret_cast<const byte*>(str), capacity);
      result.setBytes(runtime->newBytesWithAll(view));
    }
    result.setNumItems(capacity);
  }
  return ApiHandle::newReference(thread, *result);
}

PY_EXPORT PyObject* PyByteArray_FromObject(PyObject* obj) {
  Thread* thread = Thread::currentThread();
  if (obj == nullptr) {
    return ApiHandle::newReference(thread, thread->runtime()->newByteArray());
  }
  HandleScope scope(thread);
  Object src(&scope, ApiHandle::fromPyObject(obj)->asObject());
  Object result(&scope, thread->invokeFunction1(SymbolId::kBuiltins,
                                                SymbolId::kByteArray, src));
  return result.isError() ? nullptr : ApiHandle::newReference(thread, *result);
}

PY_EXPORT int PyByteArray_Resize(PyObject* pyobj, Py_ssize_t newsize) {
  DCHECK(pyobj != nullptr, "null argument to PyByteArray_Resize");
  DCHECK(newsize >= 0, "negative size");
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Object obj(&scope, ApiHandle::fromPyObject(pyobj)->asObject());
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfByteArray(*obj)) {
    thread->raiseBadArgument();
    return -1;
  }
  ByteArray array(&scope, *obj);
  word requested = static_cast<word>(newsize);
  word current = array.numItems();
  if (requested == current) return 0;
  if (requested < current) {
    array.downsize(requested);
  } else {
    runtime->byteArrayEnsureCapacity(thread, array, requested);
  }
  array.setNumItems(requested);
  return 0;
}

PY_EXPORT Py_ssize_t PyByteArray_Size(PyObject* pyobj) {
  DCHECK(pyobj != nullptr, "null argument to PyByteArray_Size");
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Object obj(&scope, ApiHandle::fromPyObject(pyobj)->asObject());
  if (!thread->runtime()->isInstanceOfByteArray(*obj)) {
    thread->raiseBadArgument();
    return -1;
  }
  return static_cast<Py_ssize_t>(ByteArray::cast(*obj)->numItems());
}

}  // namespace python

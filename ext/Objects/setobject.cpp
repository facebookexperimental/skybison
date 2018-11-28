#include "cpython-func.h"
#include "runtime.h"
#include "set-builtins.h"

namespace python {

PY_EXPORT PyObject* PyFrozenSet_New(PyObject* /* e */) {
  UNIMPLEMENTED("PyFrozenSet_New");
}

PY_EXPORT int PySet_Add(PyObject* anyset, PyObject* key) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  Object set_obj(&scope, ApiHandle::fromPyObject(anyset)->asObject());

  // TODO(T28454727): add FrozenSet
  if (!runtime->isInstanceOfSet(set_obj)) {
    // TODO(wmeehan): replace with PyErr_BadInternalCall
    thread->raiseSystemErrorWithCStr("bad argument to internal function");
    return -1;
  }

  Set set(&scope, *set_obj);
  Object key_obj(&scope, ApiHandle::fromPyObject(key)->asObject());

  if (setAdd(thread, set, key_obj)->isError()) {
    return -1;
  }

  return 0;
}

PY_EXPORT int PySet_Clear(PyObject* /* t */) { UNIMPLEMENTED("PySet_Clear"); }

PY_EXPORT int PySet_ClearFreeList() { return 0; }

PY_EXPORT int PySet_Contains(PyObject* anyset, PyObject* key) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  Object set_obj(&scope, ApiHandle::fromPyObject(anyset)->asObject());

  // TODO(T28454727): add FrozenSet
  if (!runtime->isInstanceOfSet(set_obj)) {
    // TODO(wmeehan) replace with PyErr_BadInternalCall
    thread->raiseSystemErrorWithCStr("bad argument to internal function");
    return -1;
  }

  Set set(&scope, *set_obj);
  Object key_obj(&scope, ApiHandle::fromPyObject(key)->asObject());

  return runtime->setIncludes(set, key_obj);
}

PY_EXPORT int PySet_Discard(PyObject* /* t */, PyObject* /* y */) {
  UNIMPLEMENTED("PySet_Discard");
}

PY_EXPORT PyObject* PySet_New(PyObject* iterable) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  if (iterable == nullptr) {
    return ApiHandle::newReference(thread, runtime->newSet());
  }

  Object obj(&scope, ApiHandle::fromPyObject(iterable)->asObject());
  Set set(&scope, runtime->newSet());

  Object result(&scope, runtime->setUpdate(thread, set, obj));
  if (result->isError()) {
    thread->raiseTypeErrorWithCStr("PySet_New requires an iterable");
    return nullptr;
  }

  return ApiHandle::newReference(thread, *set);
}

PY_EXPORT PyObject* PySet_Pop(PyObject* /* t */) { UNIMPLEMENTED("PySet_Pop"); }

PY_EXPORT Py_ssize_t PySet_Size(PyObject* anyset) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  Object set_obj(&scope, ApiHandle::fromPyObject(anyset)->asObject());
  // TODO(T28454727): test for FrozenSet
  if (!runtime->isInstanceOfSet(set_obj)) {
    // TODO(wmeehan) replace with PyErr_BadInternalCall
    thread->raiseSystemErrorWithCStr("bad argument to internal function");
    return -1;
  }

  Set set(&scope, *set_obj);
  return set->numItems();
}

}  // namespace python

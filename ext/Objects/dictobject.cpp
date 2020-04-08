// dictobject.c implementation

#include "cpython-func.h"

#include "capi-handles.h"
#include "dict-builtins.h"
#include "handles.h"
#include "int-builtins.h"
#include "objects.h"
#include "runtime.h"
#include "str-builtins.h"

namespace py {

PY_EXPORT PyTypeObject* PyDictItems_Type_Ptr() {
  Thread* thread = Thread::current();
  return reinterpret_cast<PyTypeObject*>(ApiHandle::borrowedReference(
      thread, thread->runtime()->typeAt(LayoutId::kDictItems)));
}

PY_EXPORT PyTypeObject* PyDictIterItem_Type_Ptr() {
  Thread* thread = Thread::current();
  return reinterpret_cast<PyTypeObject*>(ApiHandle::borrowedReference(
      thread, thread->runtime()->typeAt(LayoutId::kDictItemIterator)));
}

PY_EXPORT PyTypeObject* PyDictIterKey_Type_Ptr() {
  Thread* thread = Thread::current();
  return reinterpret_cast<PyTypeObject*>(ApiHandle::borrowedReference(
      thread, thread->runtime()->typeAt(LayoutId::kDictKeyIterator)));
}

PY_EXPORT PyTypeObject* PyDictIterValue_Type_Ptr() {
  Thread* thread = Thread::current();
  return reinterpret_cast<PyTypeObject*>(ApiHandle::borrowedReference(
      thread, thread->runtime()->typeAt(LayoutId::kDictValueIterator)));
}

PY_EXPORT PyTypeObject* PyDictKeys_Type_Ptr() {
  Thread* thread = Thread::current();
  return reinterpret_cast<PyTypeObject*>(ApiHandle::borrowedReference(
      thread, thread->runtime()->typeAt(LayoutId::kDictKeys)));
}

PY_EXPORT PyTypeObject* PyDictValues_Type_Ptr() {
  Thread* thread = Thread::current();
  return reinterpret_cast<PyTypeObject*>(ApiHandle::borrowedReference(
      thread, thread->runtime()->typeAt(LayoutId::kDictValues)));
}

PY_EXPORT int PyDict_CheckExact_Func(PyObject* obj) {
  return ApiHandle::fromPyObject(obj)->asObject().isDict();
}

PY_EXPORT int PyDict_Check_Func(PyObject* obj) {
  return Thread::current()->runtime()->isInstanceOfDict(
      ApiHandle::fromPyObject(obj)->asObject());
}

PY_EXPORT Py_ssize_t PyDict_GET_SIZE_Func(PyObject* dict) {
  HandleScope scope;
  Dict dict_obj(&scope, ApiHandle::fromPyObject(dict)->asObject());
  return dict_obj.numItems();
}

PY_EXPORT int _PyDict_SetItem_KnownHash(PyObject* pydict, PyObject* key,
                                        PyObject* value, Py_hash_t pyhash) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object dict_obj(&scope, ApiHandle::fromPyObject(pydict)->asObject());
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfDict(*dict_obj)) {
    thread->raiseBadInternalCall();
    return -1;
  }
  Dict dict(&scope, *dict_obj);
  Object key_obj(&scope, ApiHandle::fromPyObject(key)->asObject());
  Object value_obj(&scope, ApiHandle::fromPyObject(value)->asObject());
  word hash = SmallInt::truncate(pyhash);
  if (dictAtPut(thread, dict, key_obj, hash, value_obj).isErrorException()) {
    return -1;
  }
  return 0;
}

PY_EXPORT int PyDict_SetItem(PyObject* pydict, PyObject* key, PyObject* value) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object dict_obj(&scope, ApiHandle::fromPyObject(pydict)->asObject());
  if (!thread->runtime()->isInstanceOfDict(*dict_obj)) {
    thread->raiseBadInternalCall();
    return -1;
  }
  Dict dict(&scope, *dict_obj);
  Object key_obj(&scope, ApiHandle::fromPyObject(key)->asObject());
  Object value_obj(&scope, ApiHandle::fromPyObject(value)->asObject());
  Object hash_obj(&scope, Interpreter::hash(thread, key_obj));
  if (hash_obj.isError()) return -1;
  word hash = SmallInt::cast(*hash_obj).value();
  if (dictAtPut(thread, dict, key_obj, hash, value_obj).isErrorException()) {
    return -1;
  }
  return 0;
}

PY_EXPORT int PyDict_SetItemString(PyObject* pydict, const char* key,
                                   PyObject* value) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object dict_obj(&scope, ApiHandle::fromPyObject(pydict)->asObject());
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfDict(*dict_obj)) {
    thread->raiseBadInternalCall();
    return -1;
  }
  Dict dict(&scope, *dict_obj);
  Str key_obj(&scope, runtime->newStrFromCStr(key));
  Object value_obj(&scope, ApiHandle::fromPyObject(value)->asObject());
  word hash = strHash(thread, *key_obj);
  if (dictAtPut(thread, dict, key_obj, hash, value_obj).isErrorException()) {
    return -1;
  }
  return 0;
}

PY_EXPORT PyTypeObject* PyDict_Type_Ptr() {
  Thread* thread = Thread::current();
  return reinterpret_cast<PyTypeObject*>(ApiHandle::borrowedReference(
      thread, thread->runtime()->typeAt(LayoutId::kDict)));
}

PY_EXPORT PyObject* PyDict_New() {
  Thread* thread = Thread::current();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  Object value(&scope, runtime->newDict());
  return ApiHandle::newReference(thread, *value);
}

static PyObject* getItem(Thread* thread, const Object& dict_obj,
                         const Object& key) {
  HandleScope scope(thread);
  // For historical reasons, PyDict_GetItem supresses all errors that may occur.
  if (!thread->runtime()->isInstanceOfDict(*dict_obj)) {
    return nullptr;
  }
  Dict dict(&scope, *dict_obj);
  Object hash_obj(&scope, Interpreter::hash(thread, key));
  if (hash_obj.isError()) {
    thread->clearPendingException();
    return nullptr;
  }
  word hash = SmallInt::cast(*hash_obj).value();
  Object result(&scope, dictAt(thread, dict, key, hash));
  if (result.isErrorException()) {
    thread->clearPendingException();
    return nullptr;
  }
  if (result.isErrorNotFound()) {
    return nullptr;
  }
  return ApiHandle::borrowedReference(thread, *result);
}

PY_EXPORT PyObject* _PyDict_GetItem_KnownHash(PyObject* pydict, PyObject* key,
                                              Py_hash_t pyhash) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object dictobj(&scope, ApiHandle::fromPyObject(pydict)->asObject());
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfDict(*dictobj)) {
    thread->raiseBadInternalCall();
    return nullptr;
  }
  Dict dict(&scope, *dictobj);
  Object key_obj(&scope, ApiHandle::fromPyObject(key)->asObject());
  word hash = SmallInt::truncate(pyhash);
  Object value(&scope, dictAt(thread, dict, key_obj, hash));
  if (value.isError()) return nullptr;
  return ApiHandle::borrowedReference(thread, *value);
}

PY_EXPORT PyObject* PyDict_GetItem(PyObject* pydict, PyObject* key) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object dict(&scope, ApiHandle::fromPyObject(pydict)->asObject());
  Object key_obj(&scope, ApiHandle::fromPyObject(key)->asObject());
  return getItem(thread, dict, key_obj);
}

PY_EXPORT PyObject* PyDict_GetItemString(PyObject* pydict, const char* key) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object dict(&scope, ApiHandle::fromPyObject(pydict)->asObject());
  Object key_obj(&scope, thread->runtime()->newStrFromCStr(key));
  return getItem(thread, dict, key_obj);
}

PY_EXPORT void PyDict_Clear(PyObject* pydict) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object dict_obj(&scope, ApiHandle::fromPyObject(pydict)->asObject());
  if (!runtime->isInstanceOfDict(*dict_obj)) {
    return;
  }
  Dict dict(&scope, *dict_obj);
  dict.setNumItems(0);
  dict.setData(runtime->emptyTuple());
}

PY_EXPORT int PyDict_Contains(PyObject* pydict, PyObject* key) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Dict dict(&scope, ApiHandle::fromPyObject(pydict)->asObject());
  Object key_obj(&scope, ApiHandle::fromPyObject(key)->asObject());
  Object hash_obj(&scope, Interpreter::hash(thread, key_obj));
  if (hash_obj.isErrorException()) return -1;
  word hash = SmallInt::cast(*hash_obj).value();
  Object result(&scope, dictIncludes(thread, dict, key_obj, hash));
  if (result.isErrorException()) return -1;
  return Bool::cast(*result).value();
}

PY_EXPORT PyObject* PyDict_Copy(PyObject* pydict) {
  Thread* thread = Thread::current();
  if (pydict == nullptr) {
    thread->raiseBadInternalCall();
    return nullptr;
  }
  HandleScope scope(thread);
  Object dict_obj(&scope, ApiHandle::fromPyObject(pydict)->asObject());
  if (!thread->runtime()->isInstanceOfDict(*dict_obj)) {
    thread->raiseBadInternalCall();
    return nullptr;
  }
  Dict dict(&scope, *dict_obj);
  return ApiHandle::newReference(thread, dictCopy(thread, dict));
}

PY_EXPORT int PyDict_DelItem(PyObject* pydict, PyObject* key) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object dict_obj(&scope, ApiHandle::fromPyObject(pydict)->asObject());
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfDict(*dict_obj)) {
    thread->raiseBadInternalCall();
    return -1;
  }
  Dict dict(&scope, *dict_obj);
  Object key_obj(&scope, ApiHandle::fromPyObject(key)->asObject());
  Object hash_obj(&scope, Interpreter::hash(thread, key_obj));
  if (hash_obj.isErrorException()) return -1;
  word hash = SmallInt::cast(*hash_obj).value();
  if (dictRemove(thread, dict, key_obj, hash).isError()) {
    thread->raise(LayoutId::kKeyError, *key_obj);
    return -1;
  }
  return 0;
}

PY_EXPORT int PyDict_DelItemString(PyObject* pydict, const char* key) {
  PyObject* str = PyUnicode_FromString(key);
  if (str == nullptr) return -1;
  int result = PyDict_DelItem(pydict, str);
  Py_DECREF(str);
  return result;
}

PY_EXPORT PyObject* PyDict_GetItemWithError(PyObject* pydict, PyObject* key) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object dict_obj(&scope, ApiHandle::fromPyObject(pydict)->asObject());
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfDict(*dict_obj)) {
    thread->raiseBadInternalCall();
    return nullptr;
  }

  Object key_obj(&scope, ApiHandle::fromPyObject(key)->asObject());
  Object hash_obj(&scope, Interpreter::hash(thread, key_obj));
  if (hash_obj.isErrorException()) return nullptr;
  word hash = SmallInt::cast(*hash_obj).value();
  Dict dict(&scope, *dict_obj);
  Object value(&scope, dictAt(thread, dict, key_obj, hash));
  if (value.isError()) {
    return nullptr;
  }
  return ApiHandle::borrowedReference(thread, *value);
}

PY_EXPORT PyObject* PyDict_Items(PyObject* pydict) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object dict_obj(&scope, ApiHandle::fromPyObject(pydict)->asObject());
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfDict(*dict_obj)) {
    thread->raiseBadInternalCall();
    return nullptr;
  }
  Dict dict(&scope, *dict_obj);
  word len = dict.numItems();
  List result(&scope, runtime->newList());
  if (len > 0) {
    MutableTuple items(&scope, runtime->newMutableTuple(len));
    word num_items = 0;
    Object key(&scope, NoneType::object());
    Object value(&scope, NoneType::object());
    for (word i = 0; dictNextItem(dict, &i, &key, &value);) {
      Tuple kvpair(&scope, runtime->newTuple(2));
      kvpair.atPut(0, *key);
      kvpair.atPut(1, *value);
      items.atPut(num_items++, *kvpair);
    }
    result.setItems(*items);
    result.setNumItems(len);
  }
  return ApiHandle::newReference(thread, *result);
}

PY_EXPORT PyObject* PyDict_Keys(PyObject* pydict) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object dict_obj(&scope, ApiHandle::fromPyObject(pydict)->asObject());
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfDict(*dict_obj)) {
    thread->raiseBadInternalCall();
    return nullptr;
  }
  Dict dict(&scope, *dict_obj);
  return ApiHandle::newReference(thread, dictKeys(thread, dict));
}

PY_EXPORT int PyDict_Merge(PyObject* left, PyObject* right,
                           int override_matching) {
  CHECK_BOUND(override_matching, 2);
  Thread* thread = Thread::current();
  if (left == nullptr || right == nullptr) {
    thread->raiseBadInternalCall();
    return -1;
  }
  HandleScope scope(thread);
  Object left_obj(&scope, ApiHandle::fromPyObject(left)->asObject());
  if (!thread->runtime()->isInstanceOfDict(*left_obj)) {
    thread->raiseBadInternalCall();
    return -1;
  }
  Dict left_dict(&scope, *left_obj);
  Object right_obj(&scope, ApiHandle::fromPyObject(right)->asObject());
  auto merge_func = override_matching ? dictMergeOverride : dictMergeIgnore;
  if ((*merge_func)(thread, left_dict, right_obj).isError()) {
    return -1;
  }
  return 0;
}

PY_EXPORT int PyDict_MergeFromSeq2(PyObject* /* d */, PyObject* /* 2 */,
                                   int /* e */) {
  UNIMPLEMENTED("PyDict_MergeFromSeq2");
}

PY_EXPORT int _PyDict_Next(PyObject* dict, Py_ssize_t* ppos, PyObject** pkey,
                           PyObject** pvalue, Py_hash_t* phash) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object dict_obj(&scope, ApiHandle::fromPyObject(dict)->asObject());
  if (!thread->runtime()->isInstanceOfDict(*dict_obj)) {
    return 0;
  }
  Dict dict_dict(&scope, *dict_obj);
  // Below are all the possible statuses of ppos and what to do in each case.
  // * If an index is out of bounds, we should not advance.
  // * If an index does not point to a valid bucket, we should try and find the
  //   next bucket, or fail.
  // * Read the contents of that bucket.
  // * Advance the index.
  Object key(&scope, NoneType::object());
  Object value(&scope, NoneType::object());
  word hash;
  if (!dictNextItemHash(dict_dict, ppos, &key, &value, &hash)) {
    return 0;
  }
  // At this point, we will always have a valid bucket index.
  if (pkey != nullptr) *pkey = ApiHandle::borrowedReference(thread, *key);
  if (pvalue != nullptr) *pvalue = ApiHandle::borrowedReference(thread, *value);
  if (phash != nullptr) *phash = hash;
  return true;
}

PY_EXPORT int PyDict_Next(PyObject* dict, Py_ssize_t* ppos, PyObject** pkey,
                          PyObject** pvalue) {
  return _PyDict_Next(dict, ppos, pkey, pvalue, nullptr);
}

PY_EXPORT Py_ssize_t PyDict_Size(PyObject* p) {
  Thread* thread = Thread::current();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  Object dict_obj(&scope, ApiHandle::fromPyObject(p)->asObject());
  if (!runtime->isInstanceOfDict(*dict_obj)) {
    thread->raiseBadInternalCall();
    return -1;
  }

  Dict dict(&scope, *dict_obj);
  return dict.numItems();
}

PY_EXPORT int PyDict_Update(PyObject* left, PyObject* right) {
  return PyDict_Merge(left, right, 1);
}

PY_EXPORT PyObject* PyDict_Values(PyObject* pydict) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object dict_obj(&scope, ApiHandle::fromPyObject(pydict)->asObject());
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfDict(*dict_obj)) {
    thread->raiseBadInternalCall();
    return nullptr;
  }
  Dict dict(&scope, *dict_obj);
  word len = dict.numItems();
  List result(&scope, runtime->newList());
  if (len > 0) {
    MutableTuple values(&scope, runtime->newMutableTuple(len));
    word num_values = 0;
    Object value(&scope, NoneType::object());
    for (word i = 0; dictNextValue(dict, &i, &value);) {
      values.atPut(num_values++, *value);
    }
    result.setItems(*values);
    result.setNumItems(len);
  }
  return ApiHandle::newReference(thread, *result);
}

PY_EXPORT PyObject* PyObject_GenericGetDict(PyObject* /* j */, void* /* t */) {
  UNIMPLEMENTED("PyObject_GenericGetDict");
}

}  // namespace py

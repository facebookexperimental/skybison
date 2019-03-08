// dictobject.c implementation

#include "cpython-func.h"
#include "dict-builtins.h"
#include "handles.h"
#include "objects.h"
#include "runtime.h"

namespace python {

PY_EXPORT int PyDict_CheckExact_Func(PyObject* obj) {
  return ApiHandle::fromPyObject(obj)->asObject().isDict();
}

PY_EXPORT int PyDict_Check_Func(PyObject* obj) {
  return Thread::currentThread()->runtime()->isInstanceOfDict(
      ApiHandle::fromPyObject(obj)->asObject());
}

PY_EXPORT int _PyDict_SetItem_KnownHash(PyObject* pydict, PyObject* key,
                                        PyObject* value, Py_hash_t hash) {
  Thread* thread = Thread::currentThread();
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
  SmallInt hash_obj(&scope, SmallInt::fromWordTruncated(hash));
  runtime->dictAtPutWithHash(dict, key_obj, value_obj, hash_obj);
  return 0;
}

static int setItem(Thread* thread, const Object& dictobj, const Object& key,
                   const Object& value) {
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfDict(*dictobj)) {
    thread->raiseBadInternalCall();
    return -1;
  }
  HandleScope scope(thread);
  Dict dict(&scope, *dictobj);
  Object key_hash_obj(&scope,
                      thread->invokeMethod1(key, SymbolId::kDunderHash));
  if (key_hash_obj.isError()) {
    return -1;
  }
  if (!key_hash_obj.isInt()) {
    thread->raiseTypeErrorWithCStr("__hash__ method should return an integer");
    return -1;
  }
  Int large_key_hash(&scope, *key_hash_obj);
  SmallInt key_hash(&scope,
                    SmallInt::fromWordTruncated(large_key_hash.digitAt(0)));
  runtime->dictAtPutWithHash(dict, key, value, key_hash);
  return 0;
}

PY_EXPORT int PyDict_SetItem(PyObject* pydict, PyObject* key, PyObject* value) {
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Object dictobj(&scope, ApiHandle::fromPyObject(pydict)->asObject());
  Object keyobj(&scope, ApiHandle::fromPyObject(key)->asObject());
  Object valueobj(&scope, ApiHandle::fromPyObject(value)->asObject());
  return setItem(thread, dictobj, keyobj, valueobj);
}

PY_EXPORT int PyDict_SetItemString(PyObject* pydict, const char* key,
                                   PyObject* value) {
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Object dictobj(&scope, ApiHandle::fromPyObject(pydict)->asObject());
  Object keyobj(&scope, thread->runtime()->newStrFromCStr(key));
  Object valueobj(&scope, ApiHandle::fromPyObject(value)->asObject());
  return setItem(thread, dictobj, keyobj, valueobj);
}

PY_EXPORT PyObject* PyDict_New() {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  Object value(&scope, runtime->newDict());
  return ApiHandle::newReference(thread, *value);
}

static PyObject* getItemKnownHash(Thread* thread, const Dict& dict,
                                  const Object& key_obj,
                                  const Object& key_hash_obj) {
  HandleScope scope(thread);
  if (!key_hash_obj.isInt()) {
    thread->raiseTypeErrorWithCStr("__hash__ method should return an integer");
    return nullptr;
  }
  Int key_hash_int(&scope, *key_hash_obj);
  SmallInt key_hash(&scope,
                    SmallInt::fromWordTruncated(key_hash_int.digitAt(0)));
  Object value(&scope,
               thread->runtime()->dictAtWithHash(dict, key_obj, key_hash));
  if (value.isError()) return nullptr;
  return ApiHandle::borrowedReference(thread, *value);
}

static PyObject* getItem(Thread* thread, const Object& dict_obj,
                         const Object& key_obj) {
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfDict(*dict_obj)) return nullptr;

  HandleScope scope(thread);
  Dict dict(&scope, *dict_obj);
  Object key_hash(&scope,
                  thread->invokeMethod1(key_obj, SymbolId::kDunderHash));
  if (key_hash.isError()) {
    return nullptr;
  }
  return getItemKnownHash(thread, dict, key_obj, key_hash);
}

PY_EXPORT PyObject* _PyDict_GetItem_KnownHash(PyObject* pydict, PyObject* key,
                                              Py_hash_t hash) {
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Object dictobj(&scope, ApiHandle::fromPyObject(pydict)->asObject());
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfDict(*dictobj)) {
    thread->raiseBadInternalCall();
    return nullptr;
  }
  Dict dict(&scope, *dictobj);
  Object key_obj(&scope, ApiHandle::fromPyObject(key)->asObject());
  SmallInt hash_obj(&scope, SmallInt::fromWordTruncated(hash));
  return getItemKnownHash(thread, dict, key_obj, hash_obj);
}

PY_EXPORT PyObject* PyDict_GetItem(PyObject* pydict, PyObject* key) {
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Object dict(&scope, ApiHandle::fromPyObject(pydict)->asObject());
  Object key_obj(&scope, ApiHandle::fromPyObject(key)->asObject());
  return getItem(thread, dict, key_obj);
}

PY_EXPORT PyObject* PyDict_GetItemString(PyObject* pydict, const char* key) {
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Object dict(&scope, ApiHandle::fromPyObject(pydict)->asObject());
  Object key_obj(&scope, thread->runtime()->newStrFromCStr(key));
  return getItem(thread, dict, key_obj);
}

PY_EXPORT void PyDict_Clear(PyObject* pydict) {
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object dict_obj(&scope, ApiHandle::fromPyObject(pydict)->asObject());
  if (!runtime->isInstanceOfDict(*dict_obj)) {
    return;
  }
  Dict dict(&scope, *dict_obj);
  dict.setNumItems(0);
  dict.setData(runtime->newTuple(0));
}

PY_EXPORT int PyDict_ClearFreeList() { return 0; }

PY_EXPORT int PyDict_Contains(PyObject* pydict, PyObject* key) {
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Dict dict(&scope, ApiHandle::fromPyObject(pydict)->asObject());
  Object key_obj(&scope, ApiHandle::fromPyObject(key)->asObject());
  // TODO(T36757907): Return -1 when dictIncludes fails to hash the key
  return runtime->dictIncludes(dict, key_obj);
}

PY_EXPORT PyObject* PyDict_Copy(PyObject* pydict) {
  Thread* thread = Thread::currentThread();
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
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Object dict_obj(&scope, ApiHandle::fromPyObject(pydict)->asObject());
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfDict(*dict_obj)) {
    thread->raiseBadInternalCall();
    return -1;
  }
  Dict dict(&scope, *dict_obj);
  Object key_obj(&scope, ApiHandle::fromPyObject(key)->asObject());
  Frame* frame = thread->currentFrame();
  Object dunder_hash(&scope, Interpreter::lookupMethod(thread, frame, key_obj,
                                                       SymbolId::kDunderHash));
  if (dunder_hash.isNoneType()) {
    thread->raiseTypeErrorWithCStr("unhashable object");
    return -1;
  }
  Object key_hash(
      &scope, Interpreter::callMethod1(thread, frame, dunder_hash, key_obj));
  if (key_hash.isError()) {
    thread->raiseTypeErrorWithCStr("key is not hashable");
    return -1;
  }
  if (runtime->dictRemoveWithHash(dict, key_obj, key_hash).isError()) {
    thread->raiseKeyError(*key_obj);
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
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Object dict_obj(&scope, ApiHandle::fromPyObject(pydict)->asObject());
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfDict(*dict_obj)) {
    thread->raiseBadInternalCall();
    return nullptr;
  }

  Frame* frame = thread->currentFrame();
  Object key_obj(&scope, ApiHandle::fromPyObject(key)->asObject());
  Object dunder_hash(&scope, Interpreter::lookupMethod(thread, frame, key_obj,
                                                       SymbolId::kDunderHash));
  if (dunder_hash.isNoneType()) {
    thread->raiseTypeErrorWithCStr("unhashable object");
    return nullptr;
  }
  Object key_hash(
      &scope, Interpreter::callMethod1(thread, frame, dunder_hash, key_obj));
  if (key_hash.isError()) {
    thread->raiseTypeErrorWithCStr("key is not hashable");
    return nullptr;
  }
  Dict dict(&scope, *dict_obj);
  Object value(&scope, runtime->dictAtWithHash(dict, key_obj, key_hash));
  if (value.isError()) {
    return nullptr;
  }
  return ApiHandle::borrowedReference(thread, *value);
}

PY_EXPORT PyObject* PyDict_Items(PyObject* pydict) {
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Object dict_obj(&scope, ApiHandle::fromPyObject(pydict)->asObject());
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfDict(*dict_obj)) {
    thread->raiseBadInternalCall();
    return nullptr;
  }
  Dict dict(&scope, *dict_obj);
  List items(&scope, runtime->newList());
  items.setNumItems(dict.numItems());
  items.setItems(runtime->dictItems(thread, dict));
  return ApiHandle::newReference(thread, *items);
}

PY_EXPORT PyObject* PyDict_Keys(PyObject* pydict) {
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Object dict_obj(&scope, ApiHandle::fromPyObject(pydict)->asObject());
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfDict(*dict_obj)) {
    thread->raiseBadInternalCall();
    return nullptr;
  }
  Dict dict(&scope, *dict_obj);
  Tuple keys_tuple(&scope, runtime->dictKeys(dict));
  List keys(&scope, runtime->newList());
  keys.setItems(*keys_tuple);
  keys.setNumItems(keys_tuple.length());
  return ApiHandle::newReference(thread, *keys);
}

PY_EXPORT int PyDict_Merge(PyObject* left, PyObject* right,
                           int override_matching) {
  CHECK_BOUND(override_matching, 2);
  Thread* thread = Thread::currentThread();
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

PY_EXPORT int PyDict_Next(PyObject* pydict, Py_ssize_t* ppos, PyObject** pkey,
                          PyObject** pvalue) {
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Object dict_obj(&scope, ApiHandle::fromPyObject(pydict)->asObject());
  if (!thread->runtime()->isInstanceOfDict(*dict_obj)) {
    return 0;
  }
  Dict dict(&scope, *dict_obj);
  Tuple dict_data(&scope, dict.data());
  if (!Dict::Bucket::nextItem(*dict_data, ppos)) {
    return false;
  }
  if (pkey != nullptr) {
    *pkey = ApiHandle::borrowedReference(thread,
                                         Dict::Bucket::key(*dict_data, *ppos));
  }
  if (pvalue != nullptr) {
    *pvalue = ApiHandle::borrowedReference(
        thread, Dict::Bucket::value(*dict_data, *ppos));
  }
  *ppos += Dict::Bucket::kNumPointers;
  return true;
}

PY_EXPORT Py_ssize_t PyDict_Size(PyObject* p) {
  Thread* thread = Thread::currentThread();
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
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Object dict_obj(&scope, ApiHandle::fromPyObject(pydict)->asObject());
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfDict(*dict_obj)) {
    thread->raiseBadInternalCall();
    return nullptr;
  }
  Dict dict(&scope, *dict_obj);
  List values(&scope, runtime->newList());
  values.setNumItems(dict.numItems());
  values.setItems(runtime->dictValues(thread, dict));
  return ApiHandle::newReference(thread, *values);
}

PY_EXPORT PyObject* PyObject_GenericGetDict(PyObject* /* j */, void* /* t */) {
  UNIMPLEMENTED("PyObject_GenericGetDict");
}

}  // namespace python

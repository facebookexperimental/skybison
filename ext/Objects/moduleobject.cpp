// moduleobject.c implementation

#include "handles.h"
#include "objects.h"
#include "runtime.h"

namespace python {

PY_EXPORT int PyModule_CheckExact_Func(PyObject* obj) {
  return ApiHandle::fromPyObject(obj)->asObject()->isModule();
}

PY_EXPORT int PyModule_Check_Func(PyObject* obj) {
  if (PyModule_CheckExact_Func(obj)) {
    return true;
  }
  return ApiHandle::fromPyObject(obj)->isSubclass(Thread::currentThread(),
                                                  LayoutId::kModule);
}

PY_EXPORT PyObject* PyModule_Create2(struct PyModuleDef* def, int) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  const char* c_name = def->m_name;
  Object name(&scope, runtime->newStrFromCStr(c_name));
  Module module(&scope, runtime->newModule(name));
  module->setDef(runtime->newIntFromCPtr(def));

  if (def->m_doc != nullptr) {
    Object doc(&scope, runtime->newStrFromCStr(def->m_doc));
    Object key(&scope, runtime->symbols()->DunderDoc());
    runtime->moduleAtPut(module, key, doc);
  }

  ApiHandle* handle = ApiHandle::fromObject(module);
  if (def->m_size > 0) {
    handle->setCache(std::malloc(def->m_size));
  } else {
    handle->setCache(nullptr);
  }

  // TODO(eelizondo): Check m_slots
  // TODO(eelizondo): Set md_state
  // TODO(eelizondo): Validate m_methods
  // TODO(eelizondo): Add methods

  return handle;
}

PY_EXPORT PyModuleDef* PyModule_GetDef(PyObject* pymodule) {
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Object module_obj(&scope, ApiHandle::fromPyObject(pymodule)->asObject());
  if (!module_obj->isModule()) {
    // TODO(cshapiro): throw a TypeError
    return nullptr;
  }
  Module module(&scope, *module_obj);
  if (!module->def()->isInt()) {
    return nullptr;
  }
  Int def(&scope, module->def());
  return static_cast<PyModuleDef*>(def->asCPtr());
}

PY_EXPORT PyObject* PyModule_GetDict(PyObject* pymodule) {
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);

  Module module(&scope, ApiHandle::fromPyObject(pymodule)->asObject());
  return ApiHandle::fromObject(module->dict());
}

PY_EXPORT PyObject* PyModule_GetNameObject(PyObject* mod) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  Object module_obj(&scope, ApiHandle::fromPyObject(mod)->asObject());
  if (!module_obj->isModule()) {
    // TODO(atalaba): Allow for module subclassing
    thread->raiseTypeErrorWithCStr(
        "PyModule_GetNameObject takes a Module object");
    return nullptr;
  }
  Module module(&scope, *module_obj);
  Str key(&scope, runtime->symbols()->DunderName());
  Object name(&scope, runtime->moduleAt(module, key));
  if (!runtime->isInstanceOfStr(name)) {
    thread->raiseSystemErrorWithCStr("nameless module");
    return nullptr;
  }
  return ApiHandle::fromObject(name);
}

PY_EXPORT void* PyModule_GetState(PyObject* m) {
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);

  Object mod_object(&scope, ApiHandle::fromPyObject(m)->asObject());
  if (!mod_object->isModule()) {
    // TODO(atalaba): Support module subclassing
    // TODO(T36797384): Replace SystemError with PyErr_BadArgument()
    thread->raiseSystemErrorWithCStr("Bad argument to PyModule_GetState");
    return nullptr;
  }
  return ApiHandle::fromObject(mod_object)->cache();
}

PY_EXPORT PyObject* PyModuleDef_Init(struct PyModuleDef* /* f */) {
  UNIMPLEMENTED("PyModuleDef_Init");
}

PY_EXPORT int PyModule_AddFunctions(PyObject* /* m */, PyMethodDef* /* s */) {
  UNIMPLEMENTED("PyModule_AddFunctions");
}

PY_EXPORT int PyModule_ExecDef(PyObject* /* e */, PyModuleDef* /* f */) {
  UNIMPLEMENTED("PyModule_ExecDef");
}

PY_EXPORT PyObject* PyModule_FromDefAndSpec2(struct PyModuleDef* /* f */,
                                             PyObject* /* c */, int /* n */) {
  UNIMPLEMENTED("PyModule_FromDefAndSpec2");
}

PY_EXPORT const char* PyModule_GetFilename(PyObject* /* m */) {
  UNIMPLEMENTED("PyModule_GetFilename");
}

PY_EXPORT PyObject* PyModule_GetFilenameObject(PyObject* pymodule) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  Object module_obj(&scope, ApiHandle::fromPyObject(pymodule)->asObject());
  if (!module_obj->isModule()) {
    // TODO(atalaba): Allow for module subclassing
    // TODO(T36797384): Replace with PyErr_BadArgument
    thread->raiseTypeErrorWithCStr(
        "PyModule_GetFilenameObject takes a Module object");
    return nullptr;
  }
  Module module(&scope, *module_obj);
  Str key(&scope, runtime->symbols()->DunderFile());
  Object filename(&scope, runtime->moduleAt(module, key));
  if (!runtime->isInstanceOfStr(filename)) {
    thread->raiseSystemErrorWithCStr("module filename missing");
    return nullptr;
  }
  return ApiHandle::fromObject(filename);
}

PY_EXPORT const char* PyModule_GetName(PyObject* /* m */) {
  UNIMPLEMENTED("PyModule_GetName");
}

PY_EXPORT PyObject* PyModule_New(const char* c_name) {
  DCHECK(name != nullptr, "PyModule_New takes a valid string");
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  Str name(&scope, runtime->newStrFromCStr(c_name));
  return ApiHandle::fromObject(runtime->newModule(name));
}

PY_EXPORT PyObject* PyModule_NewObject(PyObject* name) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  Object name_obj(&scope, ApiHandle::fromPyObject(name)->asObject());
  Module module(&scope, runtime->newModule(name_obj));
  ApiHandle* handle = ApiHandle::fromObject(module);
  // Module's state is represented by the handle's cache
  handle->setCache(nullptr);
  return handle;
}

PY_EXPORT int PyModule_SetDocString(PyObject* m, const char* doc) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Object module_obj(&scope, ApiHandle::fromPyObject(m)->asObject());
  Object uni(&scope, runtime->newStrFromCStr(doc));
  if (!uni->isStr() || !module_obj->isModule()) {
    return -1;
  }
  Module module(&scope, *module_obj);
  Object key(&scope, runtime->symbols()->DunderDoc());
  runtime->moduleAtPut(module, key, uni);
  return 0;
}

}  // namespace python

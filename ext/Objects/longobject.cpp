// longobject.c implementation

#include "Python.h"

#include "runtime/handles.h"
#include "runtime/objects.h"
#include "runtime/runtime.h"

namespace python {

void PyLong_Type_Init(void) {
  PyTypeObject* pylong_type =
      static_cast<PyTypeObject*>(std::malloc(sizeof(PyTypeObject)));
  *pylong_type = {
      PyVarObject_HEAD_INIT(&PyType_Type, 0) "long",  // tp_name
      0,                                              // tp_basicsize
      0,                                              // tp_itemsize
      0,                                              // tp_dealloc
      0,                                              // tp_print
      0,                                              // tp_getattr
      0,                                              // tp_setattr
      0,                                              // tp_reserved
      0,                                              // tp_repr
      0,                                              // tp_as_number
      0,                                              // tp_as_sequence
      0,                                              // tp_as_mapping
      0,                                              // tp_hash
      0,                                              // tp_call
      0,                                              // tp_str
      0,                                              // tp_getattro
      0,                                              // tp_setattro
      0,                                              // tp_as_buffer
      Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_LONG_SUBCLASS |
          Py_TPFLAGS_BUILTIN,  // tp_flags
      0,                       // tp_doc
      0,                       // tp_traverse
      0,                       // tp_clear
      0,                       // tp_richcompare
      0,                       // tp_weaklistoffset
      0,                       // tp_iter
      0,                       // tp_iternext
      0,                       // tp_methods
      0,                       // tp_members
      0,                       // tp_getset
      0,                       // tp_base
      0,                       // tp_dict
      0,                       // tp_descr_get
      0,                       // tp_descr_set
      0,                       // tp_dictoffset
      0,                       // tp_init
      0,                       // tp_alloc
      0,                       // tp_new
      0,                       // tp_free
      0,                       // tp_is_gc
      0,                       // tp_bases
      0,                       // tp_mro
      0,                       // tp_cache
      0,                       // tp_subclasses
      0,                       // tp_weaklist
      0,                       // tp_del
      0,                       // tp_version_tag
      0,                       // tp_finalize
  };

  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  runtime->addBuiltinExtensionType(pylong_type);
}

extern "C" PyTypeObject* PyLong_Type_Ptr() {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  return static_cast<PyTypeObject*>(
      runtime->builtinExtensionTypes(static_cast<int>(ExtensionTypes::kLong)));
}

extern "C" PyObject* PyLong_FromLong(long ival) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  word val = reinterpret_cast<word>(ival);
  Handle<Object> value(&scope, runtime->newInteger(val));
  return ApiHandle::fromObject(*value)->asPyObject();
}

extern "C" PyObject* PyLong_FromLongLong(long long) {
  UNIMPLEMENTED("PyLong_FromLongLong");
}

extern "C" PyObject* PyLong_FromUnsignedLong(unsigned long) {
  UNIMPLEMENTED("PyLong_FromUnsignedLong");
}

extern "C" PyObject* PyLong_FromUnsignedLongLong(unsigned long long) {
  UNIMPLEMENTED("PyLong_FromUnsignedLongLong");
}

extern "C" PyObject* PyLong_FromSsize_t(Py_ssize_t) {
  UNIMPLEMENTED("PyLong_FromSsize_t");
}

extern "C" long PyLong_AsLong(PyObject*) { UNIMPLEMENTED("PyLong_AsLong"); }

extern "C" long long PyLong_AsLongLong(PyObject*) {
  UNIMPLEMENTED("PYLong_AsLongLong");
}

extern "C" unsigned long PyLong_AsUnsignedLong(PyObject*) {
  UNIMPLEMENTED("PyLong_AsUnsignedLong");
}

extern "C" unsigned long long PyLong_AsUnsignedLongLong(PyObject*) {
  UNIMPLEMENTED("PyLong_AsUnsignedLongLong");
}

extern "C" Py_ssize_t PyLong_AsSsize_t(PyObject*) {
  UNIMPLEMENTED("PyLong_AsSsize_t");
}

}  // namespace python

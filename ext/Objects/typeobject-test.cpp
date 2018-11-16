#include "capi-fixture.h"

namespace python {

using TypeExtensionApiTest = ExtensionApi;

// TODO(eelizondo): Remove once typeobject.c is compiled in
extern "C" PyObject* PyType_GenericAlloc(PyTypeObject* type,
                                         Py_ssize_t nitems) {
  // note that we need to add one, for the sentinel
  const size_t size = _PyObject_VAR_SIZE(type, nitems + 1);
  PyObject* obj = (PyObject*)PyObject_MALLOC(size);

  if (obj == NULL) {
    return PyErr_NoMemory();
  }
  memset(obj, 0, size);

  if (type->tp_flags & Py_TPFLAGS_HEAPTYPE) {
    Py_INCREF(type);
  }

  PyObject_INIT(obj, type);
  return obj;
}

TEST_F(TypeExtensionApiTest, ReadyInitializesType) {
  // Create a simple PyTypeObject
  PyTypeObject empty_type{PyObject_HEAD_INIT(NULL)};
  empty_type.tp_name = "Empty";
  empty_type.tp_flags = Py_TPFLAGS_DEFAULT;

  // EmptyType is not initialized yet
  EXPECT_FALSE(PyType_GetFlags(&empty_type) & Py_TPFLAGS_READY);

  // Run PyType_Ready
  EXPECT_EQ(0, PyType_Ready(&empty_type));

  // Expect PyTypeObject to contain the correct flags
  EXPECT_TRUE(PyType_GetFlags(&empty_type) & Py_TPFLAGS_DEFAULT);
  EXPECT_TRUE(PyType_GetFlags(&empty_type) & Py_TPFLAGS_READY);
}

TEST_F(TypeExtensionApiTest, ReadyCreatesRuntimeType) {
  // Create a simple PyTypeObject
  PyTypeObject empty_type{PyObject_HEAD_INIT(NULL)};
  empty_type.tp_name = "test.Empty";
  empty_type.tp_flags = Py_TPFLAGS_DEFAULT;
  ASSERT_EQ(PyType_Ready(&empty_type), 0);

  // Add to a module
  PyModuleDef def = {
      PyModuleDef_HEAD_INIT,
      "test",
  };
  PyObject *m, *d;
  m = PyModule_Create(&def);
  d = PyModule_GetDict(m);
  PyDict_SetItemString(d, "Empty", reinterpret_cast<PyObject*>(&empty_type));

  PyRun_SimpleString(R"(
import test
x = test.Empty
)");

  EXPECT_TRUE(PyType_CheckExact(_PyModuleGet("__main__", "x")));
}

typedef struct {
  PyObject_HEAD;
  int value;
} CustomObject;

static PyObject* Custom_new(PyTypeObject* type, PyObject* args,
                            PyObject* kwds) {
  CustomObject* self = reinterpret_cast<CustomObject*>(type->tp_alloc(type, 0));
  return reinterpret_cast<PyObject*>(self);
}

static int Custom_init(CustomObject* self, PyObject* args, PyObject* kwds) {
  self->value = 30;
  return 0;
}

static void Custom_dealloc(CustomObject* self) {
  Py_TYPE(self)->tp_free(static_cast<void*>(self));
}

TEST_F(TypeExtensionApiTest, InitializeCustomTypeInstance) {
  // Instantiate Type
  PyTypeObject custom_type{PyObject_HEAD_INIT(NULL)};
  custom_type.tp_basicsize = sizeof(CustomObject);
  custom_type.tp_name = "custom.Custom";
  custom_type.tp_flags = Py_TPFLAGS_DEFAULT;
  custom_type.tp_alloc = PyType_GenericAlloc;
  custom_type.tp_new = reinterpret_cast<newfunc>(Custom_new);
  custom_type.tp_init = reinterpret_cast<initproc>(Custom_init);
  custom_type.tp_dealloc = reinterpret_cast<destructor>(Custom_dealloc);
  custom_type.tp_free = reinterpret_cast<freefunc>(PyObject_Del);
  PyType_Ready(&custom_type);

  // Add to a module
  PyModuleDef def = {
      PyModuleDef_HEAD_INIT,
      "custom",
  };
  PyObject* pymodule = PyModule_Create(&def);
  PyObject* pymodule_dict = PyModule_GetDict(pymodule);
  PyDict_SetItemString(pymodule_dict, "Custom",
                       reinterpret_cast<PyObject*>(&custom_type));

  PyRun_SimpleString(R"(
import custom
instance1 = custom.Custom()
instance2 = custom.Custom()
)");

  // Verify the initialized value
  CustomObject* instance1 =
      reinterpret_cast<CustomObject*>(_PyModuleGet("__main__", "instance1"));
  EXPECT_EQ(instance1->value, 30);

  CustomObject* instance2 =
      reinterpret_cast<CustomObject*>(_PyModuleGet("__main__", "instance2"));
  EXPECT_EQ(instance2->value, 30);

  // Decref and dealloc custom instances
  ASSERT_EQ(Py_REFCNT(instance1), 1);
  ASSERT_EQ(Py_REFCNT(instance2), 1);
  Py_DECREF(instance2);
  Py_DECREF(instance1);
}

TEST_F(TypeExtensionApiTest, GenericAllocationReturnsMallocMemory) {
  // Instantiate Type
  PyTypeObject custom_type{PyObject_HEAD_INIT(NULL)};
  custom_type.tp_basicsize = sizeof(CustomObject);
  custom_type.tp_name = "custom.Custom";
  custom_type.tp_flags = Py_TPFLAGS_DEFAULT;
  PyType_Ready(&custom_type);

  PyObject* result = PyType_GenericAlloc(&custom_type, 0);
  EXPECT_EQ(1, Py_REFCNT(result));
}

}  // namespace python

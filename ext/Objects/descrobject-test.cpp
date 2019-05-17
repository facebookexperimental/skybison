#include "gtest/gtest.h"

#include "Python.h"
#include "capi-fixture.h"
#include "capi-testing.h"

namespace python {

using namespace testing;

using DescrExtensionApiTest = ExtensionApi;

// Create a new type with PyType_FromSpec with no methods, members, or getters
static void createEmptyBarType() {
  static PyType_Slot slots[1];
  slots[0] = {0, nullptr};
  static PyType_Spec spec;
  spec = {
      "__main__.Bar", 0, 0, Py_TPFLAGS_DEFAULT, slots,
  };
  PyObjectPtr type(PyType_FromSpec(&spec));
  ASSERT_NE(type, nullptr);
  ASSERT_EQ(PyType_CheckExact(type), 1);
  ASSERT_EQ(moduleSet("__main__", "Bar", type), 0);
}

TEST_F(DescrExtensionApiTest, ClassMethodAsDescriptorReturnsFunction) {
  ASSERT_NO_FATAL_FAILURE(createEmptyBarType());
  binaryfunc meth = [](PyObject* self, PyObject* args) {
    return PyTuple_Pack(2, self, args);
  };
  static PyMethodDef method_def;
  method_def = {"foo", meth, METH_VARARGS};
  PyObjectPtr type(moduleGet("__main__", "Bar"));
  PyObjectPtr descriptor(PyDescr_NewClassMethod(
      reinterpret_cast<PyTypeObject*>(type.get()), &method_def));
  ASSERT_NE(descriptor, nullptr);
  PyObject_SetAttrString(type, "foo", descriptor);
  PyObjectPtr func(PyObject_GetAttrString(type, "foo"));
  ASSERT_NE(func, nullptr);
  ASSERT_EQ(PyErr_Occurred(), nullptr);

  PyObjectPtr args(PyTuple_New(0));
  PyObjectPtr result(PyObject_CallObject(func, args));
  ASSERT_NE(result, nullptr);
  ASSERT_EQ(PyTuple_Check(result), 1);
  ASSERT_EQ(PyTuple_Size(result), 2);

  // self
  PyObject* arg0 = PyTuple_GetItem(result, 0);
  ASSERT_NE(arg0, nullptr);
  EXPECT_EQ(arg0, type);

  // args
  PyObject* arg1 = PyTuple_GetItem(result, 1);
  ASSERT_NE(arg1, nullptr);
  EXPECT_EQ(args, arg1);
}

TEST_F(DescrExtensionApiTest, ClassMethodAsCallableReturnsTypeAsFirstArg) {
  ASSERT_NO_FATAL_FAILURE(createEmptyBarType());
  binaryfunc meth = [](PyObject* self, PyObject* args) {
    return PyTuple_Pack(2, self, args);
  };
  static PyMethodDef method_def;
  method_def = {"foo", meth, METH_VARARGS};
  PyObjectPtr type(moduleGet("__main__", "Bar"));
  PyObjectPtr callable(PyDescr_NewClassMethod(
      reinterpret_cast<PyTypeObject*>(type.get()), &method_def));
  ASSERT_NE(callable, nullptr);

  PyObjectPtr args(PyTuple_New(1));
  Py_INCREF(type);  // SetItem steals a reference
  PyTuple_SetItem(args, 0, type);
  PyObjectPtr result(PyObject_CallObject(callable, args));
  ASSERT_NE(result, nullptr);
  ASSERT_EQ(PyTuple_Check(result), 1);
  ASSERT_EQ(PyTuple_Size(result), 2);

  // self
  PyObject* arg0 = PyTuple_GetItem(result, 0);
  ASSERT_NE(arg0, nullptr);
  EXPECT_EQ(arg0, type);

  // args
  PyObject* arg1 = PyTuple_GetItem(result, 1);
  ASSERT_NE(arg1, nullptr);
  ASSERT_EQ(PyTuple_Check(arg1), 1);
  EXPECT_EQ(PyTuple_Size(arg1), 0);
}

TEST_F(DescrExtensionApiTest, ClassMethodCallWithNoArgsRaisesTypeError) {
  ASSERT_NO_FATAL_FAILURE(createEmptyBarType());
  binaryfunc meth = [](PyObject*, PyObject*) {
    ADD_FAILURE();  // unreachable
    return Py_None;
  };
  static PyMethodDef method_def;
  method_def = {"foo", meth, METH_VARARGS};
  PyObjectPtr type(moduleGet("__main__", "Bar"));
  PyObjectPtr callable(PyDescr_NewClassMethod(
      reinterpret_cast<PyTypeObject*>(type.get()), &method_def));
  ASSERT_NE(callable, nullptr);

  PyObjectPtr args(PyTuple_New(0));
  PyObject* result = PyObject_CallObject(callable, args);
  ASSERT_EQ(result, nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(DescrExtensionApiTest, ClassMethodCallWithNonBoundClassRaisesTypeError) {
  ASSERT_NO_FATAL_FAILURE(createEmptyBarType());
  binaryfunc meth = [](PyObject*, PyObject*) {
    ADD_FAILURE();  // unreachable
    return Py_None;
  };
  static PyMethodDef method_def;
  method_def = {"foo", meth, METH_VARARGS};
  PyObjectPtr type(moduleGet("__main__", "Bar"));
  PyObjectPtr callable(PyDescr_NewClassMethod(
      reinterpret_cast<PyTypeObject*>(type.get()), &method_def));
  ASSERT_NE(callable, nullptr);

  PyObjectPtr args(PyTuple_New(1));
  PyTuple_SetItem(args, 0, PyLong_FromLong(123));
  PyObject* result = PyObject_CallObject(callable, args);
  ASSERT_EQ(result, nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

}  // namespace python

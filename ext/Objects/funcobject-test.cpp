// Copyright (c) Facebook, Inc. and its affiliates. (http://www.facebook.com)
#include "Python.h"
#include "gtest/gtest.h"

#include "capi-fixture.h"
#include "capi-testing.h"

namespace py {
namespace testing {

using FuncExtensionApiTest = ExtensionApi;

TEST_F(FuncExtensionApiTest,
       StaticMethodCallOnInstanceReturnsPassedArgFirstArg) {
  PyRun_SimpleString(R"(
def foo(x):
  return x
)");
  PyObjectPtr foo(mainModuleGet("foo"));
  PyObjectPtr static_foo(PyStaticMethod_New(foo));
  ASSERT_EQ(moduleSet("__main__", "static_foo", static_foo), 0);
  PyRun_SimpleString(R"(
class Bar:
  pass
setattr(Bar, "foo", static_foo)
bar = Bar()
result = bar.foo(123)
)");
  PyObjectPtr result(mainModuleGet("result"));
  ASSERT_NE(result, nullptr);
  ASSERT_EQ(PyLong_Check(result), 1);
  EXPECT_EQ(PyLong_AsLong(result), 123);
}

TEST_F(FuncExtensionApiTest, StaticMethodCallOnTypeReturnsPassedAsFirstArg) {
  PyRun_SimpleString(R"(
def foo(x):
  return x
)");
  PyObjectPtr foo(mainModuleGet("foo"));
  PyObjectPtr static_foo(PyStaticMethod_New(foo));
  ASSERT_EQ(moduleSet("__main__", "static_foo", static_foo), 0);
  PyRun_SimpleString(R"(
class Bar:
  pass
setattr(Bar, "foo", static_foo)
result = Bar.foo(123)
)");
  PyObjectPtr result(mainModuleGet("result"));
  ASSERT_NE(result, nullptr);
  ASSERT_EQ(PyLong_Check(result), 1);
  EXPECT_EQ(PyLong_AsLong(result), 123);
}

TEST_F(FuncExtensionApiTest, StaticMethodCallOnFreeFunctionRaisesTypeError) {
  PyRun_SimpleString(R"(
def foo(x):
  return x
)");
  PyObjectPtr foo(mainModuleGet("foo"));
  PyObjectPtr function(PyStaticMethod_New(foo));
  PyObjectPtr args(PyTuple_New(1));
  PyTuple_SetItem(args, 0, PyLong_FromLong(123));
  PyObject* result = PyObject_CallObject(function, args);
  ASSERT_EQ(result, nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(FuncExtensionApiTest, ClassMethodCallOnInstanceReturnsTypeAsFirstArg) {
  PyRun_SimpleString(R"(
def foo(cls):
  return cls
)");
  PyObjectPtr foo(mainModuleGet("foo"));
  PyObjectPtr class_foo(PyClassMethod_New(foo));
  ASSERT_EQ(moduleSet("__main__", "class_foo", class_foo), 0);
  PyRun_SimpleString(R"(
class Bar:
  pass
setattr(Bar, "foo", class_foo)
result = Bar().foo()
)");
  PyObjectPtr bar_type(mainModuleGet("Bar"));
  PyObjectPtr result(mainModuleGet("result"));
  ASSERT_NE(result, nullptr);
  ASSERT_EQ(PyType_Check(result), 1);
  EXPECT_EQ(result, bar_type);
}

TEST_F(FuncExtensionApiTest, ClassMethodCallOnTypeReturnsTypeAsFirstArg) {
  PyRun_SimpleString(R"(
def foo(cls):
  return cls
)");
  PyObjectPtr foo(mainModuleGet("foo"));
  PyObjectPtr class_foo(PyClassMethod_New(foo));
  ASSERT_EQ(moduleSet("__main__", "class_foo", class_foo), 0);
  PyRun_SimpleString(R"(
class Bar:
  pass
setattr(Bar, "foo", class_foo)
result = Bar.foo()
)");
  PyObjectPtr bar_type(mainModuleGet("Bar"));
  PyObjectPtr result(mainModuleGet("result"));
  ASSERT_NE(result, nullptr);
  ASSERT_EQ(PyType_Check(result), 1);
  EXPECT_EQ(result, bar_type);
}

TEST_F(FuncExtensionApiTest, ClassMethodCallOnFreeFunctionCallRaisesTypeError) {
  PyRun_SimpleString(R"(
def foo(cls):
  return cls
)");
  PyObjectPtr foo(mainModuleGet("foo"));
  PyObjectPtr function(PyClassMethod_New(foo));
  PyObjectPtr args(PyTuple_New(1));
  PyTuple_SetItem(args, 0, PyLong_FromLong(123));
  PyObject* result = PyObject_CallObject(function, args);
  ASSERT_EQ(result, nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

}  // namespace testing
}  // namespace py

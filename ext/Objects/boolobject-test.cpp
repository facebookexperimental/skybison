#include "gtest/gtest.h"

#include "Python.h"
#include "capi-fixture.h"
#include "capi-testing.h"

namespace py {

using BoolExtensionApiTest = ExtensionApi;

TEST_F(BoolExtensionApiTest, ConvertLongToBool) {
  // Test True
  testing::PyObjectPtr pybool_true(PyBool_FromLong(1));
  EXPECT_EQ(pybool_true, Py_True);

  // Test False
  testing::PyObjectPtr pybool_false(PyBool_FromLong(0));
  EXPECT_EQ(pybool_false, Py_False);
}

TEST_F(BoolExtensionApiTest, CheckBoolIdentity) {
  // Test Identitiy
  testing::PyObjectPtr pybool_true(PyBool_FromLong(1));
  testing::PyObjectPtr pybool1(PyBool_FromLong(2));
  EXPECT_EQ(pybool_true, pybool1);

  testing::PyObjectPtr pybool_false(PyBool_FromLong(0));
  testing::PyObjectPtr pybool2(PyBool_FromLong(0));
  EXPECT_EQ(pybool_false, pybool2);
}

}  // namespace py

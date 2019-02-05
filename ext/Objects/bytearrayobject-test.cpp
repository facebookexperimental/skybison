#include "gtest/gtest.h"

#include "Python.h"
#include "capi-fixture.h"
#include "capi-testing.h"

namespace python {

using namespace testing;

using ByteArrayExtensionApiTest = ExtensionApi;

TEST_F(ByteArrayExtensionApiTest, CheckWithBytesReturnsFalse) {
  PyObjectPtr bytes(PyBytes_FromString("hello"));
  EXPECT_FALSE(PyByteArray_CheckExact(bytes));
  EXPECT_FALSE(PyByteArray_Check(bytes));
}

TEST_F(ByteArrayExtensionApiTest, FromStringAndSizeReturnsByteArray) {
  PyObjectPtr array(PyByteArray_FromStringAndSize("hello", 5));
  EXPECT_TRUE(PyByteArray_Check(array));
  EXPECT_TRUE(PyByteArray_CheckExact(array));
}

TEST_F(ByteArrayExtensionApiTest, FromStringAndSizeWithNegativeSizeRaises) {
  ASSERT_EQ(PyByteArray_FromStringAndSize("hello", -1), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

}  // namespace python

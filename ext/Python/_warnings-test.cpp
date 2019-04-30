#include "Python.h"

#include "gmock/gmock-matchers.h"

#include "capi-fixture.h"
#include "capi-testing.h"

namespace python {

using namespace testing;

using UnderWarningsExtensionApiTest = ExtensionApi;

TEST_F(UnderWarningsExtensionApiTest, ResourceWarningIsIgnored) {
  CaptureStdStreams streams;
  EXPECT_EQ(PyErr_ResourceWarning(nullptr, 0, "%d", 0), 0);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(streams.err(), "");
}

TEST_F(UnderWarningsExtensionApiTest,
       WarnFormatWithNullCategoryPrintsRuntimeWarning) {
  CaptureStdStreams streams;
  EXPECT_EQ(PyErr_WarnFormat(nullptr, 0, "%d", 0), 0);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_THAT(streams.err(), ::testing::EndsWith("RuntimeWarning: 0\n"));
}

TEST_F(UnderWarningsExtensionApiTest,
       WarnExWithNullCategoryPrintsRuntimeWarning) {
  CaptureStdStreams streams;
  EXPECT_EQ(PyErr_WarnEx(nullptr, "bar", 0), 0);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_THAT(streams.err(), ::testing::EndsWith("RuntimeWarning: bar\n"));
}

TEST_F(UnderWarningsExtensionApiTest,
       WarnExWithNegativeStackLevelDefaultsToCurrentModule) {
  CaptureStdStreams streams;
  EXPECT_EQ(PyErr_WarnEx(PyExc_RuntimeWarning, "bar", -10), 0);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(streams.err(), "sys:1: RuntimeWarning: bar\n");
}

TEST_F(UnderWarningsExtensionApiTest,
       WarnExWithStackLevelGreaterThanDepthDefaultsToSys) {
  CaptureStdStreams streams;
  // TODO(T43609717): Determine the current stack depth with C-API and ensure
  // that this is a bigger number.
  EXPECT_EQ(PyErr_WarnEx(PyExc_RuntimeWarning, "bar", PY_SSIZE_T_MAX), 0);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(streams.err(), "sys:1: RuntimeWarning: bar\n");
}

}  // namespace python

#include "gtest/gtest.h"

#include "runtime.h"
#include "test-utils.h"
#include "warnings-module.h"

namespace python {
using namespace testing;

TEST(WarningsModuleTest, ModuleImporting) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
import _warnings
  )");
  HandleScope scope;
  RawObject warnings = moduleAt(&runtime, "__main__", "_warnings");
  EXPECT_TRUE(warnings->isModule());
}

TEST(WarningsModuleTest, WarnDoesNothing) {
  // TODO(T39431178): _warnings.warn() should actually do things.
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, R"(
import _warnings
result = _warnings.warn("something went wrong")
)");
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_TRUE(result->isNoneType());
}

TEST(WarningsModuleTest, WarnWithNoArgsRaises) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, R"(
import _warnings
_warnings.warn()
)"),
      LayoutId::kTypeError,
      "TypeError: 'warn' takes min 1 positional arguments but 0 given"));
}

TEST(WarningsModuleTest, WarnWithInvalidCategoryRaises) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
import _warnings
_warnings.warn("warning!", 1234)
)"),
                            LayoutId::kTypeError,
                            "category must be a Warning subclass"));
}

TEST(WarningsModuleTest, WarnWithLargeStacklevelRaises) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
import _warnings
_warnings.warn("hello", stacklevel=2 ** 70)
)"),
                            LayoutId::kOverflowError,
                            "Python int too large to convert to C ssize_t"));
}

TEST(WarningsModuleTest, WarnWithInvalidKwRaises) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
import _warnings
_warnings.warn("hello", stack_level=3)
  )"),
                            LayoutId::kTypeError,
                            "TypeError: invalid arguments"));
}

}  // namespace python

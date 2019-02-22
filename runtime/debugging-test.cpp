#include "gtest/gtest.h"

#include <iostream>

#include "debugging.h"
#include "test-utils.h"

namespace python {

using namespace testing;

TEST(DebuggingTests, FormatBool) {
  Runtime runtime;
  std::stringstream ss;
  ss << Bool::trueObj() << ';' << Bool::falseObj();
  EXPECT_EQ(ss.str(), "True;False");
}

TEST(DebuggingTests, FormatError) {
  Runtime runtime;
  std::stringstream ss;
  ss << Error::object();
  EXPECT_EQ(ss.str(), "Error");
}

TEST(DebuggingTests, FormatFloat) {
  Runtime runtime;
  std::stringstream ss;
  ss << runtime.newFloat(42.42);
  EXPECT_EQ(ss.str(), "0x1.535c28f5c28f6p+5");
}

TEST(DebuggingTests, FormatFunction) {
  Runtime runtime;
  HandleScope scope;
  std::stringstream ss;
  Object function(&scope, moduleAt(&runtime, "builtins", "callable"));
  ASSERT_TRUE(function.isFunction());
  ss << function;
  EXPECT_EQ(ss.str(), "<function \"callable\">");
}

TEST(DebuggingTests, FormatLargeInt) {
  Runtime runtime;
  std::stringstream ss;
  ss << runtime.newIntWithDigits({0x12345, kMaxUword});
  EXPECT_EQ(ss.str(), "largeint([0x0000000000012345, 0xffffffffffffffff])");
}

TEST(DebuggingTests, FormatLargeStr) {
  Runtime runtime;
  HandleScope scope;
  std::stringstream ss;
  Object str(&scope, runtime.newStrFromCStr("hello world"));
  EXPECT_TRUE(str.isLargeStr());
  ss << str;
  EXPECT_EQ(ss.str(), "\"hello world\"");
}

TEST(DebuggingTests, FormatList) {
  Runtime runtime;
  HandleScope scope;
  List list(&scope, runtime.newList());
  Object o0(&scope, NoneType::object());
  Object o1(&scope, runtime.newInt(17));
  runtime.listAdd(list, o0);
  runtime.listAdd(list, o1);
  std::stringstream ss;
  ss << list;
  EXPECT_EQ(ss.str(), "[None, 17]");
}

TEST(DebuggingTests, FormatNone) {
  Runtime runtime;
  std::stringstream ss;
  ss << NoneType::object();
  EXPECT_EQ(ss.str(), "None");
}

TEST(DebuggingTests, FormatObjectWithBuiltinClass) {
  Runtime runtime;
  std::stringstream ss;
  ss << runtime.notImplemented();
  EXPECT_EQ(ss.str(), "<\"NotImplementedType\" object>");
}

TEST(DebuggingTests, FormatObjectWithUserDefinedClass) {
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, R"(
class Foo:
  pass
foo = Foo()
)");
  Object foo(&scope, moduleAt(&runtime, "__main__", "foo"));
  std::stringstream ss;
  ss << foo;
  EXPECT_EQ(ss.str(), "<\"Foo\" object>");
}

TEST(DebuggingTests, FormatObjectWithUnknownType) {
  Runtime runtime;
  HandleScope scope;
  Object obj(&scope, runtime.notImplemented());
  // Phabricate a nameless type...
  RawType::cast(runtime.typeOf(*obj))->setName(NoneType::object());

  std::stringstream ss;
  std::stringstream expected;
  ss << obj;
  expected << "<object with LayoutId " << static_cast<word>(obj.layoutId())
           << ">";
  EXPECT_EQ(ss.str(), expected.str());
}

TEST(DebuggingTests, FormatSmallInt) {
  Runtime runtime;
  std::stringstream ss;
  ss << SmallInt::fromWord(-42) << ';'
     << SmallInt::fromWord(SmallInt::kMinValue) << ';'
     << SmallInt::fromWord(SmallInt::kMaxValue);
  std::stringstream expected;
  expected << "-42;" << SmallInt::kMinValue << ";" << SmallInt::kMaxValue;
  EXPECT_EQ(ss.str(), expected.str());
}

TEST(DebuggingTests, FormatSmallStr) {
  Runtime runtime;
  HandleScope scope;
  std::stringstream ss;
  Object str(&scope, runtime.newStrFromCStr("aa"));
  EXPECT_TRUE(str.isSmallStr());
  ss << str;
  EXPECT_EQ(ss.str(), "\"aa\"");
}

TEST(DebuggingTests, FormatTuple) {
  Runtime runtime;
  HandleScope scope;
  Tuple tuple(&scope, runtime.newTuple(2));
  tuple.atPut(0, Bool::trueObj());
  tuple.atPut(1, runtime.newStrFromCStr("hey"));
  std::stringstream ss;
  ss << tuple;
  EXPECT_EQ(ss.str(), "(True, \"hey\")");
}

TEST(DebuggingTests, FormatTupleWithoutElements) {
  Runtime runtime;
  std::stringstream ss;
  ss << runtime.newTuple(0);
  EXPECT_EQ(ss.str(), "()");
}

TEST(DebuggingTests, FormatTupleWithOneElement) {
  Runtime runtime;
  HandleScope scope;
  Tuple tuple(&scope, runtime.newTuple(1));
  tuple.atPut(0, runtime.newInt(77));
  std::stringstream ss;
  ss << tuple;
  EXPECT_EQ(ss.str(), "(77,)");
}

TEST(DebuggingTests, FormatType) {
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, R"(
class MyClass:
  pass
)");
  Object my_class(&scope, moduleAt(&runtime, "__main__", "MyClass"));
  std::stringstream ss;
  ss << my_class;
  EXPECT_EQ(ss.str(), "<type \"MyClass\">");
}

}  // namespace python

#include "gtest/gtest.h"

#include "gmock/gmock-matchers.h"

#include <iostream>

#include "debugging.h"
#include "test-utils.h"

namespace python {

using namespace testing;

static RawObject makeTestCode(Thread* thread) {
  Runtime* runtime = thread->runtime();
  HandleScope scope;
  const byte bytes_array[] = {100, 0, 83, 0};
  Bytes bytes(&scope, runtime->newBytesWithAll(bytes_array));
  Tuple consts(&scope, runtime->newTuple(1));
  consts.atPut(0, runtime->newStrFromCStr("const0"));
  Tuple names(&scope, runtime->newTuple(1));
  names.atPut(0, runtime->newStrFromCStr("name0"));
  Tuple varnames(&scope, runtime->newTuple(1));
  varnames.atPut(0, runtime->newStrFromCStr("variable0"));
  Tuple freevars(&scope, runtime->newTuple(1));
  freevars.atPut(0, runtime->newStrFromCStr("freevar0"));
  Tuple cellvars(&scope, runtime->newTuple(1));
  cellvars.atPut(0, runtime->newStrFromCStr("cellvar0"));
  Str filename(&scope, runtime->newStrFromCStr("filename0"));
  Str name(&scope, runtime->newStrFromCStr("name0"));
  Object lnotab(&scope, Bytes::empty());
  return runtime->newCode(1, 0, 0, 1, 0, bytes, consts, names, varnames,
                          freevars, cellvars, filename, name, 0, lnotab);
}

TEST(DebuggingTests, DumpExtendedCode) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object code(&scope, makeTestCode(thread));

  std::stringstream ss;
  dumpExtended(ss, *code);
  EXPECT_EQ(ss.str(),
            R"(code "name0":
  argcount: 1
  kwonlyargcount: 0
  nlocals: 0
  stacksize: 1
  filename: "filename0"
  consts: ("const0",)
  names: ("name0",)
  cellvars: ("cellvar0",)
  freevars: ("freevar0",)
  varnames: ("variable0",)
     0 LOAD_CONST 0
     2 RETURN_VALUE 0
)");
}

TEST(DebuggingTests, DumpExtendedFunction) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object qualname(&scope, runtime.newStrFromCStr("footype.baz"));
  Code code(&scope, makeTestCode(thread));
  Object closure(&scope, runtime.newTuple(0));
  Dict annotations(&scope, runtime.newDict());
  Object return_name(&scope, runtime.newStrFromCStr("return"));
  Object int_type(&scope, runtime.typeAt(LayoutId::kInt));
  runtime.dictAtPut(annotations, return_name, int_type);
  Dict kw_defaults(&scope, runtime.newDict());
  Object name0(&scope, runtime.newStrFromCStr("name0"));
  Object none(&scope, NoneType::object());
  runtime.dictAtPut(kw_defaults, name0, none);
  Tuple defaults(&scope, runtime.newTuple(1));
  defaults.atPut(0, runtime.newInt(-9));
  Dict globals(&scope, runtime.newDict());
  Dict builtins(&scope, runtime.newDict());
  Function func(&scope, Interpreter::makeFunction(
                            thread, qualname, code, closure, annotations,
                            kw_defaults, defaults, globals, builtins));
  func.setModule(runtime.newStrFromCStr("barmodule"));
  func.setName(runtime.newStrFromCStr("baz"));
  std::stringstream ss;
  dumpExtended(ss, *func);
  EXPECT_EQ(ss.str(), R"(function "baz":
  qualname: "footype.baz"
  module: "barmodule"
  annotations: {"return": <type "int">}
  closure: ()
  defaults: (-9,)
  kwdefaults: {"name0": None}
  code: code "name0":
    argcount: 1
    kwonlyargcount: 0
    nlocals: 0
    stacksize: 1
    filename: "filename0"
    consts: ("const0",)
    names: ("name0",)
    cellvars: ("cellvar0",)
    freevars: ("freevar0",)
    varnames: ("variable0",)
       0 LOAD_CONST 0
       2 RETURN_VALUE 0
)");
}

TEST(DebuggingTests, FormatBool) {
  Runtime runtime;
  std::stringstream ss;
  ss << Bool::trueObj() << ';' << Bool::falseObj();
  EXPECT_EQ(ss.str(), "True;False");
}

TEST(DebuggingTests, FormatBoundMethod) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class C:
  def foo():
    pass
bound_method = C().foo
)")
                   .isError());
  Object bound_method(&scope, moduleAt(&runtime, "__main__", "bound_method"));
  ASSERT_TRUE(bound_method.isBoundMethod());
  std::stringstream ss;
  ss << bound_method;
  EXPECT_EQ(ss.str(), "<bound_method \"C.foo\", <\"C\" object>>");
}

TEST(DebuggingTests, FormatCode) {
  Runtime runtime;
  HandleScope scope;
  Object name(&scope, runtime.newStrFromCStr("foobar"));
  Code code(&scope, runtime.newEmptyCode(name));
  std::stringstream ss;
  ss << code;
  EXPECT_EQ(ss.str(), "<code \"foobar\">");
}

TEST(DebuggingTests, FormatDict) {
  Runtime runtime;
  HandleScope scope;
  Dict dict(&scope, runtime.newDict());
  Object key0(&scope, runtime.newStrFromCStr("hello"));
  Object key1(&scope, NoneType::object());
  Object value0(&scope, runtime.newInt(88));
  Object value1(&scope, runtime.newTuple(0));
  runtime.dictAtPut(dict, key0, value0);
  runtime.dictAtPut(dict, key1, value1);
  std::stringstream ss;
  ss << dict;
  EXPECT_TRUE(ss.str() == R"({"hello": 88, None: ()})" ||
              ss.str() == R"({None: (), "hello": 88}")");
}

TEST(DebuggingTests, FormatError) {
  Runtime runtime;
  std::stringstream ss;
  ss << Error::error();
  EXPECT_EQ(ss.str(), "Error");

  ss.str("");
  ss << Error::exception();
  EXPECT_EQ(ss.str(), "Error<Exception>");

  ss.str("");
  ss << Error::notFound();
  EXPECT_EQ(ss.str(), "Error<NotFound>");

  ss.str("");
  ss << Error::noMoreItems();
  EXPECT_EQ(ss.str(), "Error<NoMoreItems>");

  ss.str("");
  ss << Error::outOfMemory();
  EXPECT_EQ(ss.str(), "Error<OutOfMemory>");

  ss.str("");
  ss << Error::outOfBounds();
  EXPECT_EQ(ss.str(), "Error<OutOfBounds>");
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
  EXPECT_EQ(ss.str(), R"(<function "callable">)");
}

TEST(DebuggingTests, FormatLargeInt) {
  Runtime runtime;
  std::stringstream ss;
  const uword digits[] = {0x12345, kMaxUword};
  ss << runtime.newIntWithDigits(digits);
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

TEST(DebuggingTests, FormatModule) {
  Runtime runtime;
  HandleScope scope;
  Object name(&scope, runtime.newStrFromCStr("foomodule"));
  Object module(&scope, runtime.newModule(name));
  std::stringstream ss;
  ss << module;
  EXPECT_EQ(ss.str(), R"(<module "foomodule">)");
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
  ss << NotImplementedType::object();
  EXPECT_EQ(ss.str(), R"(<"NotImplementedType" object>)");
}

TEST(DebuggingTests, FormatObjectWithUserDefinedClass) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class Foo:
  pass
foo = Foo()
)")
                   .isError());
  Object foo(&scope, moduleAt(&runtime, "__main__", "foo"));
  std::stringstream ss;
  ss << foo;
  EXPECT_EQ(ss.str(), R"(<"Foo" object>)");
}

TEST(DebuggingTests, FormatObjectWithUnknownType) {
  Runtime runtime;
  HandleScope scope;
  Object obj(&scope, NotImplementedType::object());
  // Phabricate a nameless type...
  RawType::cast(runtime.typeOf(*obj)).setName(NoneType::object());

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
  EXPECT_EQ(ss.str(), R"((True, "hey"))");
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
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class MyClass:
  pass
)")
                   .isError());
  Object my_class(&scope, moduleAt(&runtime, "__main__", "MyClass"));
  std::stringstream ss;
  ss << my_class;
  EXPECT_EQ(ss.str(), "<type \"MyClass\">");
}

TEST(DebuggingTests, FormatFrame) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
def func(arg0, arg1):
  hello = "world"
  return arg0 + arg1
)")
                   .isError());
  Function func(&scope, moduleAt(&runtime, "__main__", "func"));

  Thread* thread = Thread::current();
  Frame* root = thread->currentFrame();
  root->setVirtualPC(8);
  root->pushValue(NoneType::object());
  ASSERT_EQ(root->previousFrame(), nullptr);

  Frame* frame0 = thread->openAndLinkFrame(0, 2, 1);
  frame0->setCode(makeTestCode(thread));
  frame0->setVirtualPC(42);
  frame0->setLocal(0, runtime.newStrFromCStr("foo bar"));
  frame0->setLocal(1, runtime.newStrFromCStr("bar foo"));
  frame0->pushValue(*func);
  Frame* frame1 = thread->openAndLinkFrame(0, 3, 2);
  frame1->setVirtualPC(4);
  frame1->setCode(func.code());
  frame1->setLocal(0, runtime.newInt(-9));
  frame1->setLocal(1, runtime.newInt(17));
  frame1->setLocal(2, runtime.newStrFromCStr("world"));

  std::stringstream ss;
  ss << thread->currentFrame();
  EXPECT_EQ(ss.str(), R"(- pc: 8
  - stack:
    0: None
- pc: 42 ("filename0":0)
  code: "name0"
  - locals:
    0 "variable0": "foo bar"
    1: "bar foo"
  - stack:
    0: <function "func">
- pc: 4 ("<test string>":4)
  function: <function "func">
  - locals:
    0 "arg0": -9
    1 "arg1": 17
    2 "hello": "world"
)");
}

TEST(DebuggingTests, FormatFrameNullptr) {
  Runtime runtime;
  std::stringstream ss;
  ss << static_cast<Frame*>(nullptr);
  EXPECT_EQ(ss.str(), "<nullptr>");
}

}  // namespace python

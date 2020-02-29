#include <memory>

#include "gtest/gtest.h"

#include "capi-handles.h"
#include "dict-builtins.h"
#include "frame.h"
#include "function-builtins.h"
#include "int-builtins.h"
#include "modules.h"
#include "runtime.h"
#include "test-utils.h"

namespace py {
namespace testing {

using CallTest = RuntimeFixture;
using TrampolinesTest = RuntimeFixture;

TEST_F(CallTest, CallBoundMethod) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def func(self):
  return self

def test(callable):
  return callable()
)")
                   .isError());

  Object function(&scope, mainModuleAt(runtime_, "func"));
  ASSERT_TRUE(function.isFunction());

  Object self(&scope, SmallInt::fromWord(1111));
  BoundMethod method(&scope, runtime_->newBoundMethod(function, self));

  Object test(&scope, mainModuleAt(runtime_, "test"));
  ASSERT_TRUE(test.isFunction());
  Function func(&scope, *test);

  Object result(&scope, Interpreter::callFunction1(
                            thread_, thread_->currentFrame(), func, method));
  EXPECT_TRUE(isIntEqualsWord(*result, 1111));
}

TEST_F(CallTest, CallBoundMethodWithArgs) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def func(self, a, b):
  return [self, a, b]

def test(callable):
  return callable(2222, 3333)
)")
                   .isError());

  Object function(&scope, mainModuleAt(runtime_, "func"));
  ASSERT_TRUE(function.isFunction());

  Object self(&scope, SmallInt::fromWord(1111));
  BoundMethod method(&scope, runtime_->newBoundMethod(function, self));

  Object test(&scope, mainModuleAt(runtime_, "test"));
  ASSERT_TRUE(test.isFunction());
  Function func(&scope, *test);

  Object result(&scope, Interpreter::callFunction1(
                            thread_, thread_->currentFrame(), func, method));
  EXPECT_PYLIST_EQ(result, {1111, 2222, 3333});
}

TEST_F(CallTest, CallBoundMethodKw) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
result_self = None
result_a = None
result_b = None

def func(self, a, b):
  global result_self, result_a, result_b
  result_self = self
  result_a = a
  result_b = b

def test(callable):
  return callable(a=2222, b=3333)
)")
                   .isError());

  Object function(&scope, mainModuleAt(runtime_, "func"));
  ASSERT_TRUE(function.isFunction());

  Object self(&scope, SmallInt::fromWord(1111));
  BoundMethod method(&scope, runtime_->newBoundMethod(function, self));

  Object test(&scope, mainModuleAt(runtime_, "test"));
  ASSERT_TRUE(test.isFunction());
  Function func(&scope, *test);
  Tuple args(&scope, runtime_->newTuple(1));
  args.atPut(0, *method);
  callFunction(func, args);

  Object result_self(&scope, mainModuleAt(runtime_, "result_self"));
  EXPECT_TRUE(isIntEqualsWord(*result_self, 1111));

  Object result_a(&scope, mainModuleAt(runtime_, "result_a"));
  EXPECT_TRUE(isIntEqualsWord(*result_a, 2222));

  Object result_b(&scope, mainModuleAt(runtime_, "result_b"));
  EXPECT_TRUE(isIntEqualsWord(*result_b, 3333));
}

TEST_F(CallTest, CallBoundMethodExArgs) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
result_self = None
result_a = None
result_b = None

def func(self, a, b):
  global result_self, result_a, result_b
  result_self = self
  result_a = a
  result_b = b

def test(callable):
  args = (2222, 3333)
  return callable(*args)
)")
                   .isError());

  Object function(&scope, mainModuleAt(runtime_, "func"));
  ASSERT_TRUE(function.isFunction());

  Object self(&scope, SmallInt::fromWord(1111));
  BoundMethod method(&scope, runtime_->newBoundMethod(function, self));

  Object test(&scope, mainModuleAt(runtime_, "test"));
  ASSERT_TRUE(test.isFunction());
  Function func(&scope, *test);
  Tuple args(&scope, runtime_->newTuple(1));
  args.atPut(0, *method);
  callFunction(func, args);

  Object result_self(&scope, mainModuleAt(runtime_, "result_self"));
  EXPECT_TRUE(isIntEqualsWord(*result_self, 1111));

  Object result_a(&scope, mainModuleAt(runtime_, "result_a"));
  EXPECT_TRUE(isIntEqualsWord(*result_a, 2222));

  Object result_b(&scope, mainModuleAt(runtime_, "result_b"));
  EXPECT_TRUE(isIntEqualsWord(*result_b, 3333));
}

TEST_F(CallTest, CallBoundMethodExKwargs) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
result_self = None
result_a = None
result_b = None

def func(self, a, b):
  global result_self, result_a, result_b
  result_self = self
  result_a = a
  result_b = b

def test(callable):
  kwargs = {'a': 2222, 'b': 3333}
  return callable(**kwargs)
)")
                   .isError());

  Object function(&scope, mainModuleAt(runtime_, "func"));
  ASSERT_TRUE(function.isFunction());

  Object self(&scope, SmallInt::fromWord(1111));
  BoundMethod method(&scope, runtime_->newBoundMethod(function, self));

  Object test(&scope, mainModuleAt(runtime_, "test"));
  ASSERT_TRUE(test.isFunction());
  Function func(&scope, *test);
  Tuple args(&scope, runtime_->newTuple(1));
  args.atPut(0, *method);
  callFunction(func, args);

  Object result_self(&scope, mainModuleAt(runtime_, "result_self"));
  EXPECT_TRUE(isIntEqualsWord(*result_self, 1111));

  Object result_a(&scope, mainModuleAt(runtime_, "result_a"));
  EXPECT_TRUE(isIntEqualsWord(*result_a, 2222));

  Object result_b(&scope, mainModuleAt(runtime_, "result_b"));
  EXPECT_TRUE(isIntEqualsWord(*result_b, 3333));
}

TEST_F(CallTest, CallBoundMethodExArgsAndKwargs) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
result_self = None
result_a = None
result_b = None

def func(self, a, b):
  global result_self, result_a, result_b
  result_self = self
  result_a = a
  result_b = b

def test(callable):
  args = (2222,)
  kwargs = {'b': 3333}
  return callable(*args, **kwargs)
)")
                   .isError());

  Object function(&scope, mainModuleAt(runtime_, "func"));
  ASSERT_TRUE(function.isFunction());

  Object self(&scope, SmallInt::fromWord(1111));
  BoundMethod method(&scope, runtime_->newBoundMethod(function, self));

  Object test(&scope, mainModuleAt(runtime_, "test"));
  ASSERT_TRUE(test.isFunction());
  Function func(&scope, *test);
  Tuple args(&scope, runtime_->newTuple(1));
  args.atPut(0, *method);
  callFunction(func, args);

  Object result_self(&scope, mainModuleAt(runtime_, "result_self"));
  EXPECT_TRUE(isIntEqualsWord(*result_self, 1111));

  Object result_a(&scope, mainModuleAt(runtime_, "result_a"));
  EXPECT_TRUE(isIntEqualsWord(*result_a, 2222));

  Object result_b(&scope, mainModuleAt(runtime_, "result_b"));
  EXPECT_TRUE(isIntEqualsWord(*result_b, 3333));
}

TEST_F(CallTest, CallDefaultArgs) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def foo(a=1, b=2, c=3):
  return [a, b, c]

result0 = foo(33, 22, 11)
result1 = foo()
result2 = foo(1001)
result3 = foo(1001, 1002)
result4 = foo(1001, 1002, 1003)
)")
                   .isError());
  HandleScope scope(thread_);
  Object result0(&scope, mainModuleAt(runtime_, "result0"));
  EXPECT_PYLIST_EQ(result0, {33, 22, 11});
  Object result1(&scope, mainModuleAt(runtime_, "result1"));
  EXPECT_PYLIST_EQ(result1, {1, 2, 3});
  Object result2(&scope, mainModuleAt(runtime_, "result2"));
  EXPECT_PYLIST_EQ(result2, {1001, 2, 3});
  Object result3(&scope, mainModuleAt(runtime_, "result3"));
  EXPECT_PYLIST_EQ(result3, {1001, 1002, 3});
  Object result4(&scope, mainModuleAt(runtime_, "result4"));
  EXPECT_PYLIST_EQ(result4, {1001, 1002, 1003});
}

TEST_F(CallTest, CallMethodMixPosDefaultArgs) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def foo(a, b=2):
  return [a, b]
result = foo(1)
)")
                   .isError());
  HandleScope scope(thread_);
  Object result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_PYLIST_EQ(result, {1, 2});
}

TEST_F(CallTest, CallBoundMethodMixed) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class R:
  def m(self, a, b=2):
    return [a, b]
r = R()
result = r.m(9)
)")
                   .isError());
  HandleScope scope(thread_);
  Object result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_PYLIST_EQ(result, {9, 2});
}

TEST_F(CallTest, SingleKW) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def foo(bar):
   return bar
result = foo(bar=2)
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(runtime_, "result"), 2));
}

TEST_F(CallTest, MixedKW) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def foo(a, b, c):
   return [a, b, c]
result = foo(1, b = 2, c = 3)
)")
                   .isError());
  HandleScope scope(thread_);
  Object result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_PYLIST_EQ(result, {1, 2, 3});
}

TEST_F(CallTest, FullKW) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def foo(a, b, c):
   return [a, b, c]
result = foo(a = 1, b = 2, c = 3)
)")
                   .isError());
  HandleScope scope(thread_);
  Object result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_PYLIST_EQ(result, {1, 2, 3});
}

TEST_F(CallTest, KWOutOfOrder1) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def foo(a, b, c):
   return [a, b, c]
result = foo(c = 3, a = 1, b = 2)
)")
                   .isError());
  HandleScope scope(thread_);
  Object result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_PYLIST_EQ(result, {1, 2, 3});
}

TEST_F(CallTest, KWOutOfOrder2) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def foo(a, b, c):
   return [a, b, c]
result = foo(1, c = 3, b = 2)
)")
                   .isError());
  HandleScope scope(thread_);
  Object result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_PYLIST_EQ(result, {1, 2, 3});
}

TEST_F(CallTest, KeywordOnly1) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def foo(a,b, *, c):
  return [a,b,c]
result = foo(1, 2, c = 3)
)")
                   .isError());
  HandleScope scope(thread_);
  Object result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_PYLIST_EQ(result, {1, 2, 3});
}

TEST_F(CallTest, KeywordOnly2) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def foo(a,b, *, c):
  return [a,b,c]
result = foo(1, b = 2, c = 3)
)")
                   .isError());
  HandleScope scope(thread_);
  Object result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_PYLIST_EQ(result, {1, 2, 3});
}

TEST_F(CallTest, KeyWordDefaults) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def foo(a, b = 22, c = 33):
  return [a,b,c]
result = foo(11, c = 3)
)")
                   .isError());
  HandleScope scope(thread_);
  Object result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_PYLIST_EQ(result, {11, 22, 3});
}

TEST_F(CallTest, VarArgsWithExcess) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def foo(a, b, *c):
  return [a,b,c]
result = foo(1,2,3,4,5,6)
)")
                   .isError());
  HandleScope scope(thread_);
  List result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_TRUE(isIntEqualsWord(result.at(0), 1));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 2));
  Tuple tuple(&scope, result.at(2));
  ASSERT_EQ(tuple.length(), 4);
  EXPECT_TRUE(isIntEqualsWord(tuple.at(0), 3));
  EXPECT_TRUE(isIntEqualsWord(tuple.at(1), 4));
  EXPECT_TRUE(isIntEqualsWord(tuple.at(2), 5));
  EXPECT_TRUE(isIntEqualsWord(tuple.at(3), 6));
}

TEST_F(CallTest, VarArgsEmpty) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def foo(a, b, *c):
  return [a,b,c]
result = foo(1,2)
)")
                   .isError());
  HandleScope scope(thread_);
  List result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_TRUE(isIntEqualsWord(result.at(0), 1));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 2));
  Tuple tuple(&scope, result.at(2));
  EXPECT_EQ(tuple.length(), 0);
}

TEST_F(CallTest, CallWithKeywordsCalleeWithVarkeyword) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def foo(a,b,c,**d):
    return [a,b,c,d]
result = foo(1,2,c=3,g=4,h=5,j="bar")
)")
                   .isError());
  HandleScope scope(thread_);
  List result(&scope, testing::mainModuleAt(runtime_, "result"));
  EXPECT_TRUE(isIntEqualsWord(result.at(0), 1));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 2));
  EXPECT_TRUE(isIntEqualsWord(result.at(2), 3));

  Dict dict(&scope, result.at(3));
  Str name_g(&scope, runtime_->newStrFromCStr("g"));
  EXPECT_TRUE(isIntEqualsWord(dictAtByStr(thread_, dict, name_g), 4));
  Str name_h(&scope, runtime_->newStrFromCStr("h"));
  EXPECT_TRUE(isIntEqualsWord(dictAtByStr(thread_, dict, name_h), 5));
  Str name_j(&scope, runtime_->newStrFromCStr("j"));
  EXPECT_TRUE(isStrEqualsCStr(dictAtByStr(thread_, dict, name_j), "bar"));
}

TEST_F(CallTest, CallWithNoArgsCalleeDefaultArgsVarargsVarkeyargs) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def bar(a=1, b=2, *c, **d):
    return [a,b,c,d]
result = bar()
)")
                   .isError());
  HandleScope scope(thread_);
  List result(&scope, testing::mainModuleAt(runtime_, "result"));
  EXPECT_TRUE(isIntEqualsWord(result.at(0), 1));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 2));
  Tuple tuple(&scope, result.at(2));
  EXPECT_EQ(tuple.length(), 0);
  Dict dict(&scope, result.at(3));
  EXPECT_EQ(dict.numItems(), 0);
}

TEST_F(CallTest, CallPositionalCalleeVargsEmptyVarkeyargs) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def bar(a=1, b=2, *c, **d):
    return [a,b,c,d]
result = bar(1,2,3,4,5,6,7)
)")
                   .isError());
  HandleScope scope(thread_);
  List result(&scope, testing::mainModuleAt(runtime_, "result"));
  EXPECT_TRUE(isIntEqualsWord(result.at(0), 1));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 2));
  Tuple tuple(&scope, result.at(2));
  ASSERT_EQ(tuple.length(), 5);
  EXPECT_TRUE(isIntEqualsWord(tuple.at(0), 3));
  EXPECT_TRUE(isIntEqualsWord(tuple.at(1), 4));
  EXPECT_TRUE(isIntEqualsWord(tuple.at(2), 5));
  EXPECT_TRUE(isIntEqualsWord(tuple.at(3), 6));
  EXPECT_TRUE(isIntEqualsWord(tuple.at(4), 7));
  Dict dict(&scope, result.at(3));
  EXPECT_EQ(dict.numItems(), 0);
}

TEST_F(CallTest, CallWithKeywordsCalleeEmptyVarargsFullVarkeyargs) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def bar(a=1, b=2, *c, **d):
    return [a,b,c,d]
result = bar(a1=11, a2=12, a3=13)
)")
                   .isError());
  HandleScope scope(thread_);
  List result(&scope, testing::mainModuleAt(runtime_, "result"));
  EXPECT_TRUE(isIntEqualsWord(result.at(0), 1));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 2));

  Object tuple_obj(&scope, result.at(2));
  ASSERT_TRUE(tuple_obj.isTuple());
  Tuple tuple(&scope, *tuple_obj);
  EXPECT_EQ(tuple.length(), 0);

  Dict dict(&scope, result.at(3));
  Str name0(&scope, runtime_->newStrFromCStr("a3"));
  EXPECT_TRUE(isIntEqualsWord(dictAtByStr(thread_, dict, name0), 13));
  Str name1(&scope, runtime_->newStrFromCStr("a1"));
  EXPECT_TRUE(isIntEqualsWord(dictAtByStr(thread_, dict, name1), 11));
  Str name2(&scope, runtime_->newStrFromCStr("a2"));
  EXPECT_TRUE(isIntEqualsWord(dictAtByStr(thread_, dict, name2), 12));
}

TEST_F(CallTest, CallWithKeywordsCalleeFullVarargsFullVarkeyargs) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def bar(a=1, b=2, *c, **d):
    return [a,b,c,d]
result = bar(1,2,3,4,5,6,7,a9=9)
)")
                   .isError());
  HandleScope scope(thread_);
  List result(&scope, testing::mainModuleAt(runtime_, "result"));
  EXPECT_TRUE(isIntEqualsWord(result.at(0), 1));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 2));

  Tuple tuple(&scope, result.at(2));
  EXPECT_TRUE(isIntEqualsWord(tuple.at(0), 3));
  EXPECT_TRUE(isIntEqualsWord(tuple.at(1), 4));
  EXPECT_TRUE(isIntEqualsWord(tuple.at(2), 5));
  EXPECT_TRUE(isIntEqualsWord(tuple.at(3), 6));
  EXPECT_TRUE(isIntEqualsWord(tuple.at(4), 7));

  Dict dict(&scope, result.at(3));
  Str name_g(&scope, runtime_->newStrFromCStr("a9"));
  EXPECT_TRUE(isIntEqualsWord(dictAtByStr(thread_, dict, name_g), 9));
}

TEST_F(CallTest, CallWithOutOfOrderKeywords) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def foobar(a,b,*,c):
    return [a,b,c]
result = foobar(c=3,a=1,b=2)
)")
                   .isError());
  HandleScope scope(thread_);
  Object result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_PYLIST_EQ(result, {1, 2, 3});
}

TEST_F(CallTest, CallWithKeywordsCalleeVarargsKeywordOnly) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def foobar1(a,b,*c,d):
    return [a,b,c,d]
result = foobar1(1,2,3,4,5,d=9)
)")
                   .isError());
  HandleScope scope(thread_);
  List result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_TRUE(isIntEqualsWord(result.at(0), 1));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 2));
  Tuple tuple(&scope, result.at(2));
  ASSERT_EQ(tuple.length(), 3);
  EXPECT_TRUE(isIntEqualsWord(tuple.at(0), 3));
  EXPECT_TRUE(isIntEqualsWord(tuple.at(1), 4));
  EXPECT_TRUE(isIntEqualsWord(tuple.at(2), 5));
  EXPECT_TRUE(isIntEqualsWord(result.at(3), 9));
}

TEST_F(CallTest, CallWithKeywordsCalleeVarargsVarkeyargsKeywordOnly) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def foobar2(a,b,*c, e, **d):
    return [a,b,c,d,e]
result = foobar2(1,e=9,b=2,f1="a",f11=12)
)")
                   .isError());
  HandleScope scope(thread_);
  List result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_TRUE(isIntEqualsWord(result.at(0), 1));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 2));
  Tuple tuple(&scope, result.at(2));
  ASSERT_EQ(tuple.length(), 0);
  Dict dict(&scope, result.at(3));
  Str f1(&scope, runtime_->newStrFromCStr("f1"));
  EXPECT_TRUE(isStrEqualsCStr(dictAtByStr(thread_, dict, f1), "a"));
  Str f11(&scope, runtime_->newStrFromCStr("f11"));
  EXPECT_TRUE(isIntEqualsWord(dictAtByStr(thread_, dict, f11), 12));
  EXPECT_TRUE(isIntEqualsWord(result.at(4), 9));
}

TEST_F(CallTest, CallEx) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def foo(a,b,c,d):
    return [a,b,c,d]
a = (1,2,3,4)
result = foo(*a)
)")
                   .isError());
  HandleScope scope(thread_);
  Object result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_PYLIST_EQ(result, {1, 2, 3, 4});
}

TEST_F(CallTest, CallExBuildTupleUnpackWithCall) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def foo(a,b,c,d):
    return [a,b,c,d]
a = (3,4)
result = foo(1,2,*a)
)")
                   .isError());
  HandleScope scope(thread_);
  Object result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_PYLIST_EQ(result, {1, 2, 3, 4});
}

TEST_F(CallTest, CallExKw) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def foo(a,b,c,d):
    return [a,b,c,d]
a = {'d': 4, 'b': 2, 'a': 1, 'c': 3}
result = foo(**a)
)")
                   .isError());
  HandleScope scope(thread_);
  Object result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_PYLIST_EQ(result, {1, 2, 3, 4});
}

TEST_F(CallTest, KeywordOnly) {
  const char* src = R"(
def foo(a,b, *, c):
  print(a,b,c)
foo(1, 2, 3);
)";
  EXPECT_TRUE(raised(runFromCStr(runtime_, src), LayoutId::kTypeError));
}

TEST_F(CallTest, MissingKeyword) {
  const char* src = R"(
def foo(a,b, *, c):
  print(a,b,c)
foo(1, 2);
)";
  EXPECT_TRUE(raised(runFromCStr(runtime_, src), LayoutId::kTypeError));
}

TEST_F(CallTest, ArgNameMismatch) {
  const char* src = R"(
def foo(a,b, *, c):
  print(a,b,c)
foo(1, d = 2, c = 3);
)";
  EXPECT_TRUE(raised(runFromCStr(runtime_, src), LayoutId::kTypeError));
}

TEST_F(CallTest, TooManyKWArgs) {
  const char* src = R"(
def foo(a,b, *, c):
  print(a,b,c)
foo(1, 2, 4, c = 3);
)";
  EXPECT_TRUE(raised(runFromCStr(runtime_, src), LayoutId::kTypeError));
}

TEST_F(CallTest, TooManyArgs) {
  const char* src = R"(
def foo(a,b, c):
  print(a,b,c)
foo(1, 2, 3, 4);
)";
  EXPECT_TRUE(raised(runFromCStr(runtime_, src), LayoutId::kTypeError));
}

TEST_F(CallTest, TooFewArgs) {
  const char* src = R"(
def foo(a,b, c):
  print(a,b,c)
foo(3, 4);
)";
  EXPECT_TRUE(raised(runFromCStr(runtime_, src), LayoutId::kTypeError));
}

static RawObject builtinReturnSecondArg(Thread* /* thread */, Frame* frame,
                                        word nargs) {
  Arguments args(frame, nargs);
  return args.get(1);
}

static void createAndPatchBuiltinReturnSecondArg(Runtime* runtime) {
  // Ensure we have a __main__ module.
  ASSERT_FALSE(runFromCStr(runtime, "").isError());
  // def dummy(first, second):
  const char* parameter_names[] = {"first", "second"};
  addBuiltin("dummy", builtinReturnSecondArg, parameter_names, 0);
}

TEST_F(TrampolinesTest, BuiltinTrampolineKwPassesKwargs) {
  HandleScope scope(thread_);
  createAndPatchBuiltinReturnSecondArg(runtime_);
  ASSERT_FALSE(runFromCStr(runtime_, "result = dummy(second=12345, first=None)")
                   .isError());
  Object result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_TRUE(isIntEqualsWord(*result, 12345));
}

TEST_F(TrampolinesTest, BuiltinTrampolineKwWithInvalidArgRaisesTypeError) {
  createAndPatchBuiltinReturnSecondArg(runtime_);
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(runtime_, "dummy(third=3, first=1)"), LayoutId::kTypeError,
      "dummy() got an unexpected keyword argument 'third'"));
}

TEST_F(TrampolinesTest, InterpreterClosureUsesArgOverCellValue) {
  HandleScope scope(thread_);

  // Create code object
  word nlocals = 1;
  Tuple varnames(&scope, runtime_->newTuple(nlocals));
  Tuple cellvars(&scope, runtime_->newTuple(1));
  Object bar(&scope, Runtime::internStrFromCStr(thread_, "bar"));
  varnames.atPut(0, *bar);
  cellvars.atPut(0, *bar);
  const byte bytecode[] = {LOAD_CLOSURE, 0, LOAD_DEREF, 0, RETURN_VALUE, 0};
  Bytes bc(&scope, runtime_->newBytesWithAll(bytecode));
  Tuple empty_tuple(&scope, runtime_->emptyTuple());
  Object empty_str(&scope, Str::empty());
  Object empty_bytes(&scope, Bytes::empty());
  word flags = Code::Flags::kOptimized | Code::Flags::kNewlocals;
  Code code(&scope,
            runtime_->newCode(/*argcount=*/1, /*posonlyargcount=*/0,
                              /*kwonlyargcount=*/0, nlocals, /*stacksize=*/0,
                              flags, /*code=*/bc, /*consts=*/empty_tuple,
                              /*names=*/empty_tuple, varnames,
                              /*freevars=*/empty_tuple, cellvars,
                              /*filename=*/empty_str, /*name=*/empty_str,
                              /*firstlineno=*/0, /*lnotab=*/empty_bytes));
  ASSERT_TRUE(!code.cell2arg().isNoneType());

  Object qualname(&scope, runtime_->newStrFromCStr("foo"));
  Module module(&scope, runtime_->findOrCreateMainModule());
  Function foo(&scope,
               runtime_->newFunctionWithCode(thread_, qualname, code, module));
  Tuple closure_tuple(&scope, runtime_->newTuple(1));
  closure_tuple.atPut(0, runtime_->newInt(99));
  foo.setClosure(*closure_tuple);

  Object argument(&scope, runtime_->newInt(3));
  EXPECT_TRUE(
      isIntEqualsWord(Interpreter::callFunction1(
                          thread_, thread_->currentFrame(), foo, argument),
                      3));
}

TEST_F(TrampolinesTest, InterpreterClosureUsesCellValue) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def foo(arg):
  def bar():
    return arg * 3
  arg = 5
  return bar()

result = foo(-2)
)")
                   .isError());
  Function foo(&scope, mainModuleAt(runtime_, "foo"));
  ASSERT_EQ(foo.entry(), &interpreterClosureTrampoline);
  // Ensure that cellvar was populated.
  Code code(&scope, foo.code());
  ASSERT_TRUE(!code.cell2arg().isNoneType());
  Tuple cellvars(&scope, code.cellvars());
  ASSERT_EQ(cellvars.length(), 1);
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(runtime_, "result"), 15));
}

static RawObject makeFunctionWithPosOnlyArg(Thread* thread) {
  // Create:
  //   def foo(a, /, b):
  //     return (a, b)
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object name(&scope, Runtime::internStrFromCStr(thread, "foo"));
  const byte bytecode[] = {LOAD_FAST,   0, LOAD_FAST,    1,
                           BUILD_TUPLE, 2, RETURN_VALUE, 0};
  Tuple varnames(&scope, runtime->newTuple(2));
  varnames.atPut(0, Runtime::internStrFromCStr(thread, "a"));
  varnames.atPut(1, Runtime::internStrFromCStr(thread, "b"));
  Bytes bc(&scope, runtime->newBytesWithAll(bytecode));
  Object empty_tuple(&scope, runtime->emptyTuple());
  Object empty_str(&scope, Str::empty());
  Object empty_bytes(&scope, Bytes::empty());
  Code code(&scope,
            runtime->newCode(/*argcount=*/2, /*posonlyargcount=*/1,
                             /*kwonlyargcount=*/0, /*nlocals=*/2,
                             /*stacksize=*/2,
                             Code::Flags::kNewlocals | Code::Flags::kOptimized,
                             bc, /*consts=*/empty_tuple,
                             /*names=*/empty_tuple, varnames,
                             /*freevars=*/empty_tuple,
                             /*cellvars=*/empty_tuple,
                             /*filename=*/empty_str, name,
                             /*firstlineno=*/0, /*lnotab=*/empty_bytes));
  Module module(&scope, runtime->findOrCreateMainModule());
  return runtime->newFunctionWithCode(thread, name, code, module);
}

TEST_F(TrampolinesTest, KeywordCallRejectsPositionalOnlyArgumentNames) {
  HandleScope scope(thread_);
  Function function(&scope, makeFunctionWithPosOnlyArg(thread_));

  // `foo(a=2, b=4)`
  Frame* frame = thread_->currentFrame();
  frame->pushValue(*function);
  frame->pushValue(runtime_->newInt(2));
  frame->pushValue(runtime_->newInt(4));
  Tuple keywords(&scope, runtime_->newTuple(2));
  keywords.atPut(0, Runtime::internStrFromCStr(thread_, "a"));
  keywords.atPut(1, Runtime::internStrFromCStr(thread_, "b"));
  frame->pushValue(*keywords);
  Object result_obj(&scope, Interpreter::callKw(thread_, frame, 2));
  EXPECT_TRUE(raisedWithStr(
      *result_obj, LayoutId::kTypeError,
      "keyword argument specified for positional-only argument 'a'"));
}

TEST_F(TrampolinesTest, KeywordCallAcceptsNonPositionalOnlyArgumentNames) {
  HandleScope scope(thread_);
  Function function(&scope, makeFunctionWithPosOnlyArg(thread_));

  // `foo(2, b=9)`
  Frame* frame = thread_->currentFrame();
  frame->pushValue(*function);
  frame->pushValue(runtime_->newInt(2));
  frame->pushValue(runtime_->newInt(9));
  Tuple keywords(&scope, runtime_->newTuple(1));
  keywords.atPut(0, Runtime::internStrFromCStr(thread_, "b"));
  frame->pushValue(*keywords);
  Object result_obj(&scope, Interpreter::callKw(thread_, frame, 2));
  ASSERT_TRUE(result_obj.isTuple());
  Tuple result(&scope, *result_obj);
  ASSERT_EQ(result.length(), 2);
  EXPECT_TRUE(isIntEqualsWord(result.at(0), 2));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 9));
}

TEST_F(TrampolinesTest, KeywordCallWithPositionalOnlyArgumentsAndVarKeyArgs) {
  HandleScope scope(thread_);

  // Create:
  //   def foo(a, b=7, /, c=10, **kwargs):
  //     return (a, b, c, kwargs)
  Object name(&scope, Runtime::internStrFromCStr(thread_, "foo"));
  const byte bytecode[] = {LOAD_FAST, 0, LOAD_FAST,   1, LOAD_FAST,    2,
                           LOAD_FAST, 3, BUILD_TUPLE, 4, RETURN_VALUE, 0};
  Tuple varnames(&scope, runtime_->newTuple(4));
  varnames.atPut(0, Runtime::internStrFromCStr(thread_, "a"));
  varnames.atPut(1, Runtime::internStrFromCStr(thread_, "b"));
  varnames.atPut(2, Runtime::internStrFromCStr(thread_, "c"));
  varnames.atPut(3, Runtime::internStrFromCStr(thread_, "kwargs"));
  Bytes bc(&scope, runtime_->newBytesWithAll(bytecode));
  Object empty_tuple(&scope, runtime_->emptyTuple());
  Object empty_str(&scope, Str::empty());
  Object empty_bytes(&scope, Bytes::empty());
  Code code(&scope, runtime_->newCode(
                        /*argcount=*/3, /*posonlyargcount=*/2,
                        /*kwonlyargcount=*/0, /*nlocals=*/4,
                        /*stacksize=*/4,
                        Code::Flags::kNewlocals | Code::Flags::kOptimized |
                            Code::Flags::kVarkeyargs,
                        bc,
                        /*consts=*/empty_tuple,
                        /*names=*/empty_tuple, varnames,
                        /*freevars=*/empty_tuple,
                        /*cellvars=*/empty_tuple,
                        /*filename=*/empty_str, name,
                        /*firstlineno=*/0, /*lnotab=*/empty_bytes));
  Module module(&scope, runtime_->findOrCreateMainModule());
  Function foo(&scope,
               runtime_->newFunctionWithCode(thread_, name, code, module));
  Tuple defaults(&scope, runtime_->newTuple(2));
  defaults.atPut(0, runtime_->newInt(7));
  defaults.atPut(1, runtime_->newInt(10));
  foo.setDefaults(*defaults);
  // Call foo(1, c=13, b=5).
  Frame* frame = thread_->currentFrame();
  frame->pushValue(*foo);
  frame->pushValue(runtime_->newInt(1));
  frame->pushValue(runtime_->newInt(13));
  frame->pushValue(runtime_->newInt(5));
  Tuple keywords(&scope, runtime_->newTuple(2));
  keywords.atPut(0, Runtime::internStrFromCStr(thread_, "c"));
  keywords.atPut(1, Runtime::internStrFromCStr(thread_, "b"));
  frame->pushValue(*keywords);
  Object result_obj(&scope, Interpreter::callKw(thread_, frame, 3));

  // Expect a `(1, 7, 13, {'b': 5})` result.
  ASSERT_TRUE(result_obj.isTuple());
  Tuple result(&scope, *result_obj);
  ASSERT_EQ(result.length(), 4);
  EXPECT_TRUE(isIntEqualsWord(result.at(0), 1));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 7));
  EXPECT_TRUE(isIntEqualsWord(result.at(2), 13));
  ASSERT_TRUE(result.at(3).isDict());
  Dict result_dict(&scope, result.at(3));
  EXPECT_EQ(result_dict.numItems(), 1);
  Object b_name(&scope, Runtime::internStrFromCStr(thread_, "b"));
  EXPECT_TRUE(isIntEqualsWord(dictAtByStr(thread_, result_dict, b_name), 5));
}

TEST_F(TrampolinesTest, ExplodeCallWithBadKeywordFails) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, R"(
def take_kwargs(a): pass

kwargs = {12: 34}
take_kwargs(**kwargs)
)"),
                            LayoutId::kTypeError, "keywords must be strings"));
}

TEST_F(TrampolinesTest, ExplodeCallWithZeroKeywords) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def foo(a=10): return a
result = foo(**{})
)")
                   .isError());
  HandleScope scope(thread_);
  Object result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_EQ(result, SmallInt::fromWord(10));
}

static RawObject numArgs(Thread*, Frame*, word nargs) {
  return SmallInt::fromWord(nargs);
}

static void createAndPatchBuiltinNumArgs(Runtime* runtime) {
  // Ensure we have a __main__ module.
  ASSERT_FALSE(runFromCStr(runtime, "").isError());
  // def dummy(first, second):
  const char* parameter_names[] = {"first", "second"};
  addBuiltin("dummy", numArgs, parameter_names, 0);
}

TEST_F(TrampolinesTest, BuiltinTrampolineExReceivesExArgs) {
  createAndPatchBuiltinNumArgs(runtime_);
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, "result = dummy(*(1,2))").isError());
  Object result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_TRUE(isIntEqualsWord(*result, 2));
}

TEST_F(TrampolinesTest, BuiltinTrampolineExReceivesMixOfPositionalAndExArgs1) {
  createAndPatchBuiltinNumArgs(runtime_);
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, "result = dummy(1, *(2,))").isError());
  Object result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_TRUE(isIntEqualsWord(*result, 2));
}

static void createAndPatchBuiltinNumArgsVariadic(Runtime* runtime) {
  // Ensure we have a __main__ module.
  ASSERT_FALSE(runFromCStr(runtime, "").isError());
  // def dummy(*args):
  const char* parameter_names[] = {"args"};
  addBuiltin("dummy", numArgs, parameter_names, Code::Flags::kVarargs);
}

TEST_F(TrampolinesTest,
       BuiltinTrampolineExReceivesOnePositionalArgAndTwoVariableArgs) {
  createAndPatchBuiltinNumArgsVariadic(runtime_);
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, "result = dummy(1, *(2, 3))").isError());
  Object result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_TRUE(isIntEqualsWord(*result, 1));
}

static void createAndPatchBuiltinNumArgsArgsKwargs(Runtime* runtime) {
  // Ensure we have a __main__ module.
  ASSERT_FALSE(runFromCStr(runtime, "").isError());
  // def dummy(*args, **kwargs):
  const char* parameter_names[] = {"args", "kwargs"};
  addBuiltin("dummy", numArgs, parameter_names,
             Code::Flags::kVarargs | Code::Flags::kVarkeyargs);
}

TEST_F(TrampolinesTest,
       BuiltinTrampolineExReceivesTwoPositionalOneVariableAndTwoKwArgs) {
  createAndPatchBuiltinNumArgsArgsKwargs(runtime_);
  HandleScope scope(thread_);
  ASSERT_FALSE(
      runFromCStr(runtime_,
                  "result = dummy(1, 2, *(3,), **{'foo': 1, 'bar': 2})")
          .isError());
  Object result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_TRUE(isIntEqualsWord(*result, 2));
}

TEST_F(TrampolinesTest, BuiltinTrampolineExReceivesVarArgs) {
  createAndPatchBuiltinNumArgs(runtime_);
  HandleScope scope(thread_);
  ASSERT_FALSE(
      runFromCStr(runtime_, "result = dummy(*(1,), second=5)").isError());
  Object result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_TRUE(isIntEqualsWord(*result, 2));
}

TEST_F(TrampolinesTest, BuiltinTrampolineExWithTooFewArgsRaisesTypeError) {
  createAndPatchBuiltinNumArgs(runtime_);
  EXPECT_TRUE(
      raisedWithStr(runFromCStr(runtime_, "dummy(*(1,))"), LayoutId::kTypeError,
                    "'dummy' takes min 2 positional arguments but 1 given"));
}

TEST_F(TrampolinesTest, BuiltinTrampolineExWithTooManyArgsRaisesTypeError) {
  createAndPatchBuiltinNumArgs(runtime_);
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(runtime_, "dummy(*(1,2,3,4,5))"), LayoutId::kTypeError,
      "'dummy' takes max 2 positional arguments but 5 given"));
}

TEST_F(TrampolinesTest, CallFunctionExWithNamedArgAndExplodeKwargs) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def f(description, conflict_handler):
    return [description, conflict_handler]

result = f(description="foo", **{"conflict_handler": "conflict_handler value"})
)")
                   .isError());
  HandleScope scope(thread_);
  Object result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_PYLIST_EQ(result, {"foo", "conflict_handler value"});
}

TEST_F(TrampolinesTest, CallFunctionExWithExplodeKwargsStrSubclassAlwaysEq) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C(str):
    def __eq__(self, other):
        return True
    __hash__ = str.__hash__

def f(param):
    return param

actual = C("foo")
result = f(**{actual: 5})
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(runtime_, "result"), 5));
}

TEST_F(TrampolinesTest,
       CallFunctionExWithExplodeKwargsStrSubclassReturnNonBool) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, R"(
class D:
    def __bool__(self):
        raise UserWarning('foo')

class C(str):
    def __eq__(self, other):
        return D()
    __hash__ = str.__hash__

def f(param):
    return param

actual = C("foo")
result = f(**{actual: 5})
)"),
                            LayoutId::kUserWarning, "foo"));
}

TEST_F(TrampolinesTest,
       CallFunctionExWithExplodeKwargsStrSubclassRaiseException) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, R"(
class C(str):
    def __eq__(self, other):
        raise UserWarning('foo')
    __hash__ = str.__hash__

def f(param):
    return param

actual = C("foo")
result = f(**{actual: 5})
)"),
                            LayoutId::kUserWarning, "foo"));
}

TEST_F(TrampolinesTest, CallFunctionExWithExplodeKwargsStrSubclassNotEq) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, R"(
class C(str):
    __hash__ = str.__hash__

def f(param):
    return param

actual = C("foo")
result = f(**{actual: 5})
)"),
                            LayoutId::kTypeError,
                            "f() got an unexpected keyword argument 'foo'"));
}

TEST_F(TrampolinesTest, CallFunctionExWithExplodeKwargsStrSubclassNeverEq) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, R"(
class C(str):
    def __eq__(self, other):
        return False
    __hash__ = str.__hash__

def f(param):
    return param

actual = C("param")
result = f(**{actual: 5})
)"),
                            LayoutId::kTypeError,
                            "f() got an unexpected keyword argument 'param'"));
}

TEST_F(TrampolinesTest, CallFunctionWithParameterInVarnames) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, R"(
def add_argument(*args, **kwargs):
    action = action_class(**kwargs)

def init():
    add_argument(action='help')

init()
)"),
                            LayoutId::kNameError,
                            "name 'action_class' is not defined"));
}

TEST_F(TrampolinesTest, CallFunctionWithParameterInVarargname) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def test(*args, **kwargs):
    return kwargs['args']

result = test(args=5)
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(runtime_, "result"), 5));
}

TEST_F(TrampolinesTest, CallFunctionWithPositionalArgAndParameterInVarargname) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def test(pos, *args, **kwargs):
    return kwargs['args']

result = test(1, args=5)
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(runtime_, "result"), 5));
}

static PyObject* capiFunctionNoArgs(PyObject* self, PyObject* args) {
  Thread* thread = Thread::current();
  thread->runtime()->collectGarbage();
  EXPECT_TRUE(ApiHandle::hasExtensionReference(self));
  EXPECT_EQ(args, nullptr);
  return ApiHandle::newReference(thread, SmallInt::fromWord(1234));
}

TEST_F(TrampolinesTest, MethodTrampolineNoArgs) {
  HandleScope scope(thread_);
  Frame* frame = thread_->currentFrame();
  Function function(&scope, newEmptyFunction());
  function.setCode(
      runtime_->newIntFromCPtr(reinterpret_cast<void*>(capiFunctionNoArgs)));
  frame->pushValue(*function);
  Object self(&scope, runtime_->newTuple(1));
  frame->pushValue(*self);
  Object result(&scope, methodTrampolineNoArgs(thread_, frame, 1));
  EXPECT_TRUE(isIntEqualsWord(*result, 1234));
}

TEST_F(TrampolinesTest, MethodTrampolineNoArgsKw) {
  HandleScope scope(thread_);
  Frame* frame = thread_->currentFrame();
  Function function(&scope, newEmptyFunction());
  function.setCode(
      runtime_->newIntFromCPtr(reinterpret_cast<void*>(capiFunctionNoArgs)));
  frame->pushValue(*function);
  frame->pushValue(runtime_->emptyTuple());
  Object result(&scope, methodTrampolineNoArgsKw(thread_, frame, 0));
  EXPECT_TRUE(isIntEqualsWord(*result, 1234));
}

TEST_F(TrampolinesTest, MethodTrampolineNoArgsEx) {
  HandleScope scope(thread_);
  Frame* frame = thread_->currentFrame();
  Function function(&scope, newEmptyFunction());
  function.setCode(
      runtime_->newIntFromCPtr(reinterpret_cast<void*>(capiFunctionNoArgs)));
  frame->pushValue(*function);
  Tuple varargs(&scope, runtime_->newTuple(1));
  varargs.atPut(0, runtime_->newTuple(1));  // self
  frame->pushValue(*varargs);
  Object result(&scope, methodTrampolineNoArgsEx(thread_, frame, 0));
  EXPECT_TRUE(isIntEqualsWord(*result, 1234));
}

static PyObject* capiFunctionOneArg(PyObject* self, PyObject* args) {
  Thread* thread = Thread::current();
  thread->runtime()->collectGarbage();
  EXPECT_TRUE(ApiHandle::hasExtensionReference(self));
  EXPECT_TRUE(ApiHandle::hasExtensionReference(args));
  return ApiHandle::newReference(thread, SmallInt::fromWord(1234));
}

TEST_F(TrampolinesTest, MethodTrampolineOneArg) {
  HandleScope scope(thread_);
  Frame* frame = thread_->currentFrame();
  Function function(&scope, newEmptyFunction());
  function.setCode(
      runtime_->newIntFromCPtr(reinterpret_cast<void*>(capiFunctionOneArg)));
  frame->pushValue(*function);
  Object self(&scope, runtime_->newTuple(1));
  frame->pushValue(*self);
  Object args(&scope, runtime_->newTuple(1));
  frame->pushValue(*args);
  Object result(&scope, methodTrampolineOneArg(thread_, frame, 2));
  EXPECT_TRUE(isIntEqualsWord(*result, 1234));
}

TEST_F(TrampolinesTest, MethodTrampolineOneArgKw) {
  HandleScope scope(thread_);
  Frame* frame = thread_->currentFrame();
  Function function(&scope, newEmptyFunction());
  function.setCode(
      runtime_->newIntFromCPtr(reinterpret_cast<void*>(capiFunctionOneArg)));
  frame->pushValue(*function);
  Object arg(&scope, runtime_->newTuple(1));
  frame->pushValue(*arg);
  Object self(&scope, runtime_->newTuple(1));
  frame->pushValue(*self);
  Tuple kwargs(&scope, runtime_->newTuple(0));
  frame->pushValue(*kwargs);
  Object result(&scope, methodTrampolineOneArgKw(thread_, frame, 2));
  EXPECT_TRUE(isIntEqualsWord(*result, 1234));
}

TEST_F(TrampolinesTest, MethodTrampolineOneArgEx) {
  HandleScope scope(thread_);
  Frame* frame = thread_->currentFrame();
  Function function(&scope, newEmptyFunction());
  function.setCode(
      runtime_->newIntFromCPtr(reinterpret_cast<void*>(capiFunctionOneArg)));
  frame->pushValue(*function);
  Tuple varargs(&scope, runtime_->newTuple(2));
  varargs.atPut(0, runtime_->newTuple(1));  // self
  varargs.atPut(1, runtime_->newTuple(1));  // arg
  frame->pushValue(*varargs);
  Object result(&scope, methodTrampolineOneArgEx(thread_, frame, 0));
  EXPECT_TRUE(isIntEqualsWord(*result, 1234));
}

static PyObject* capiFunctionVarArgs(PyObject* self, PyObject* args) {
  Thread* thread = Thread::current();
  thread->runtime()->collectGarbage();
  EXPECT_TRUE(ApiHandle::hasExtensionReference(self));
  EXPECT_TRUE(ApiHandle::hasExtensionReference(args));
  return ApiHandle::newReference(thread, SmallInt::fromWord(1234));
}

TEST_F(TrampolinesTest, MethodTrampolineVarArgs) {
  HandleScope scope(thread_);
  Frame* frame = thread_->currentFrame();
  Function function(&scope, newEmptyFunction());
  function.setCode(
      runtime_->newIntFromCPtr(reinterpret_cast<void*>(capiFunctionVarArgs)));
  frame->pushValue(*function);
  Object self(&scope, runtime_->newTuple(1));
  frame->pushValue(*self);
  Object arg(&scope, runtime_->newTuple(1));
  frame->pushValue(*arg);
  Object result(&scope, methodTrampolineVarArgs(thread_, frame, 2));
  EXPECT_TRUE(isIntEqualsWord(*result, 1234));
}

TEST_F(TrampolinesTest, MethodTrampolineVarArgsKw) {
  HandleScope scope(thread_);
  Frame* frame = thread_->currentFrame();
  Function function(&scope, newEmptyFunction());
  function.setCode(
      runtime_->newIntFromCPtr(reinterpret_cast<void*>(capiFunctionVarArgs)));
  frame->pushValue(*function);
  Object self(&scope, runtime_->newTuple(1));
  frame->pushValue(*self);
  Object arg0(&scope, runtime_->newTuple(1));
  frame->pushValue(*arg0);
  Object arg1(&scope, runtime_->newTuple(1));
  frame->pushValue(*arg1);
  Object kwargs(&scope, runtime_->newTuple(0));
  frame->pushValue(*kwargs);
  Object result(&scope, methodTrampolineVarArgsKw(thread_, frame, 3));
  EXPECT_TRUE(isIntEqualsWord(*result, 1234));
}

TEST_F(TrampolinesTest, MethodTrampolineVarArgsEx) {
  HandleScope scope(thread_);
  Frame* frame = thread_->currentFrame();
  Function function(&scope, newEmptyFunction());
  function.setCode(
      runtime_->newIntFromCPtr(reinterpret_cast<void*>(capiFunctionVarArgs)));
  frame->pushValue(*function);
  Tuple varargs(&scope, runtime_->newTuple(1));
  Object self(&scope, runtime_->newTuple(1));
  varargs.atPut(0, *self);
  frame->pushValue(*varargs);
  Object result(&scope, methodTrampolineVarArgsEx(thread_, frame, 0));
  EXPECT_TRUE(isIntEqualsWord(*result, 1234));
}

static PyObject* capiFunctionKeywordsNullKwargs(PyObject* self, PyObject* args,
                                                PyObject* kwargs) {
  Thread* thread = Thread::current();
  thread->runtime()->collectGarbage();
  EXPECT_TRUE(ApiHandle::hasExtensionReference(self));
  EXPECT_TRUE(ApiHandle::hasExtensionReference(args));
  EXPECT_EQ(kwargs, nullptr);
  return ApiHandle::newReference(thread, SmallInt::fromWord(1234));
}

TEST_F(TrampolinesTest, MethodTrampolineKeywords) {
  HandleScope scope(thread_);
  Frame* frame = thread_->currentFrame();
  Function function(&scope, newEmptyFunction());
  function.setCode(runtime_->newIntFromCPtr(
      reinterpret_cast<void*>(capiFunctionKeywordsNullKwargs)));
  frame->pushValue(*function);
  Object self(&scope, runtime_->newTuple(1));
  frame->pushValue(*self);
  Object result(&scope, methodTrampolineKeywords(thread_, frame, 1));
  EXPECT_TRUE(isIntEqualsWord(*result, 1234));
}

static PyObject* capiFunctionKeywords(PyObject* self, PyObject* args,
                                      PyObject* kwargs) {
  Thread* thread = Thread::current();
  thread->runtime()->collectGarbage();
  EXPECT_TRUE(ApiHandle::hasExtensionReference(self));
  EXPECT_TRUE(ApiHandle::hasExtensionReference(args));
  EXPECT_TRUE(ApiHandle::hasExtensionReference(kwargs));
  return ApiHandle::newReference(thread, SmallInt::fromWord(1234));
}

TEST_F(TrampolinesTest, MethodTrampolineKeywordsKw) {
  HandleScope scope(thread_);
  Frame* frame = thread_->currentFrame();
  Function function(&scope, newEmptyFunction());
  function.setCode(
      runtime_->newIntFromCPtr(reinterpret_cast<void*>(capiFunctionKeywords)));
  frame->pushValue(*function);
  Object self(&scope, runtime_->newTuple(1));
  frame->pushValue(*self);
  frame->pushValue(SmallStr::fromCStr("bar"));
  Tuple kwnames(&scope, runtime_->newTuple(1));
  kwnames.atPut(0, SmallStr::fromCStr("foo"));
  frame->pushValue(*kwnames);
  Object result(&scope, methodTrampolineKeywordsKw(thread_, frame, 2));
  EXPECT_TRUE(isIntEqualsWord(*result, 1234));
}

TEST_F(TrampolinesTest, MethodTrampolineKeywordsEx) {
  HandleScope scope(thread_);
  Frame* frame = thread_->currentFrame();
  Function function(&scope, newEmptyFunction());
  function.setCode(runtime_->newIntFromCPtr(
      reinterpret_cast<void*>(capiFunctionKeywordsNullKwargs)));
  frame->pushValue(*function);
  Tuple varargs(&scope, runtime_->newTuple(1));
  Object self(&scope, runtime_->newTuple(1));
  varargs.atPut(0, *self);
  frame->pushValue(*varargs);
  Object result(&scope, methodTrampolineKeywordsEx(thread_, frame, 0));
  EXPECT_TRUE(isIntEqualsWord(*result, 1234));
}

static PyObject* capiFunctionFastCallNullKwnames(PyObject* self,
                                                 PyObject** args, word nargs,
                                                 PyObject* kwnames) {
  Thread* thread = Thread::current();
  thread->runtime()->collectGarbage();
  EXPECT_TRUE(ApiHandle::hasExtensionReference(self));
  for (word i = 0; i < nargs; i++) {
    EXPECT_TRUE(ApiHandle::hasExtensionReference(args[i]))
        << "Expected fastcall arg #" << i << " to be owned by the trampoline";
  }
  EXPECT_EQ(kwnames, nullptr);
  return ApiHandle::newReference(thread, SmallInt::fromWord(1234));
}

TEST_F(TrampolinesTest, MethodTrampolineFastCall) {
  HandleScope scope(thread_);
  Frame* frame = thread_->currentFrame();
  Function function(&scope, newEmptyFunction());
  function.setCode(runtime_->newIntFromCPtr(
      reinterpret_cast<void*>(capiFunctionFastCallNullKwnames)));
  frame->pushValue(*function);
  Object self(&scope, runtime_->newTuple(1));
  frame->pushValue(*self);
  Object arg0(&scope, runtime_->newTuple(1));
  frame->pushValue(*arg0);
  Object result(&scope, methodTrampolineFastCall(thread_, frame, 2));
  EXPECT_TRUE(isIntEqualsWord(*result, 1234));
}

static PyObject* capiFunctionFastCall(PyObject* self, PyObject** args,
                                      word nargs, PyObject* kwnames) {
  Thread* thread = Thread::current();
  thread->runtime()->collectGarbage();
  EXPECT_TRUE(ApiHandle::hasExtensionReference(self));
  word num_keywords =
      Tuple::cast(ApiHandle::fromPyObject(kwnames)->asObject()).length();
  for (word i = 0; i < nargs + num_keywords; i++) {
    EXPECT_TRUE(ApiHandle::hasExtensionReference(args[i]))
        << "Expected fastcall arg #" << i << " to be owned by the trampoline";
  }
  EXPECT_TRUE(ApiHandle::hasExtensionReference(kwnames));
  return ApiHandle::newReference(thread, SmallInt::fromWord(1234));
}

TEST_F(TrampolinesTest, MethodTrampolineFastCallKw) {
  HandleScope scope(thread_);
  Frame* frame = thread_->currentFrame();
  Function function(&scope, newEmptyFunction());
  function.setCode(
      runtime_->newIntFromCPtr(reinterpret_cast<void*>(capiFunctionFastCall)));
  frame->pushValue(*function);
  Object self(&scope, runtime_->newTuple(1));
  frame->pushValue(*self);
  Object kwarg0(&scope, runtime_->newTuple(1));
  frame->pushValue(*kwarg0);
  Tuple kwnames(&scope, runtime_->newTuple(1));
  kwnames.atPut(0, SmallStr::fromCStr("foo"));
  frame->pushValue(*kwnames);
  Object result(&scope, methodTrampolineFastCallKw(thread_, frame, 2));
  EXPECT_TRUE(isIntEqualsWord(*result, 1234));
}

TEST_F(TrampolinesTest, MethodTrampolineFastCallEx) {
  HandleScope scope(thread_);
  Frame* frame = thread_->currentFrame();
  Function function(&scope, newEmptyFunction());
  function.setCode(
      runtime_->newIntFromCPtr(reinterpret_cast<void*>(capiFunctionFastCall)));
  frame->pushValue(*function);
  Tuple varargs(&scope, runtime_->newTuple(2));
  Object self(&scope, runtime_->newTuple(1));
  varargs.atPut(0, *self);
  varargs.atPut(1, SmallStr::fromCStr("bar"));
  frame->pushValue(*varargs);
  Dict varkeywords(&scope, runtime_->newDict());
  Str key(&scope, SmallStr::fromCStr("baz"));
  Object value(&scope, runtime_->newTuple(1));
  dictAtPutByStr(thread_, varkeywords, key, value);
  frame->pushValue(*varkeywords);
  Object result(&scope, methodTrampolineFastCallEx(
                            thread_, frame, CallFunctionExFlag::VAR_KEYWORDS));
  EXPECT_TRUE(isIntEqualsWord(*result, 1234));
}

}  // namespace testing
}  // namespace py

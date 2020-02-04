#include "bytearray-builtins.h"

#include "gtest/gtest.h"

#include "builtins.h"
#include "test-utils.h"

namespace py {

using namespace testing;

using ByteArrayBuiltinsTest = RuntimeFixture;

TEST_F(ByteArrayBuiltinsTest, Add) {
  HandleScope scope(thread_);

  ByteArray array(&scope, runtime_->newByteArray());
  byteArrayAdd(thread_, runtime_, array, 0);
  byteArrayAdd(thread_, runtime_, array, 1);
  byteArrayAdd(thread_, runtime_, array, 2);
  EXPECT_GE(array.capacity(), 3);
  EXPECT_EQ(array.numItems(), 3);
  EXPECT_EQ(array.byteAt(0), 0);
  EXPECT_EQ(array.byteAt(1), 1);
  EXPECT_EQ(array.byteAt(2), 2);
}

TEST_F(ByteArrayBuiltinsTest, AsBytes) {
  HandleScope scope(thread_);

  ByteArray array(&scope, runtime_->newByteArray());
  Bytes bytes(&scope, byteArrayAsBytes(thread_, runtime_, array));
  EXPECT_TRUE(isBytesEqualsBytes(bytes, View<byte>(nullptr, 0)));

  array.setBytes(runtime_->mutableBytesWith(10, 0));
  array.setNumItems(3);
  bytes = byteArrayAsBytes(thread_, runtime_, array);
  const byte expected_bytes[] = {0, 0, 0};
  EXPECT_TRUE(isBytesEqualsBytes(bytes, expected_bytes));
}

TEST_F(ByteArrayBuiltinsTest, ClearSetsLengthToZero) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
array = bytearray(b'foo')
array.clear()
)")
                   .isError());
  ByteArray array(&scope, mainModuleAt(runtime_, "array"));
  EXPECT_EQ(array.numItems(), 0);
}

TEST_F(ByteArrayBuiltinsTest, DunderAddWithNonByteArraySelfRaisesTypeError) {
  EXPECT_TRUE(
      raisedWithStr(runFromCStr(runtime_, "bytearray.__add__(b'', b'')"),
                    LayoutId::kTypeError,
                    "'__add__' requires a 'bytearray' object but got 'bytes'"));
}

TEST_F(ByteArrayBuiltinsTest, DunderAddWithNonBytesLikeRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(runtime_, "bytearray(b'') + None"), LayoutId::kTypeError,
      "can only concatenate bytearray or bytes to bytearray"));
}

TEST_F(ByteArrayBuiltinsTest, DunderAddWithByteArrayOtherReturnsNewByteArray) {
  HandleScope scope(thread_);
  ByteArray self(&scope, runtime_->newByteArray());
  ByteArray other(&scope, runtime_->newByteArray());
  const byte byte_array[] = {'1', '2', '3'};
  runtime_->byteArrayExtend(thread_, other, byte_array);
  Object result(&scope, runBuiltin(METH(bytearray, __add__), self, other));
  EXPECT_TRUE(isByteArrayEqualsCStr(self, ""));
  EXPECT_TRUE(isByteArrayEqualsCStr(result, "123"));
}

TEST_F(ByteArrayBuiltinsTest, DunderAddWithBytesOtherReturnsNewByteArray) {
  HandleScope scope;
  ByteArray self(&scope, runtime_->newByteArray());
  Bytes other(&scope, runtime_->newBytes(4, '1'));
  Object result(&scope, runBuiltin(METH(bytearray, __add__), self, other));
  EXPECT_TRUE(isByteArrayEqualsCStr(self, ""));
  EXPECT_TRUE(isByteArrayEqualsCStr(result, "1111"));
}

TEST_F(ByteArrayBuiltinsTest,
       DunderAddWithBytesSubclassOtherReturnsNewByteArray) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class Foo(bytes): pass
other = Foo(b"1234")
)")
                   .isError());
  HandleScope scope;
  Object self(&scope, runtime_->newByteArray());
  Object other(&scope, mainModuleAt(runtime_, "other"));
  Object result(&scope, runBuiltin(METH(bytearray, __add__), self, other));
  EXPECT_TRUE(isByteArrayEqualsCStr(self, ""));
  EXPECT_TRUE(isByteArrayEqualsCStr(result, "1234"));
}

TEST_F(ByteArrayBuiltinsTest, DunderAddReturnsConcatenatedByteArray) {
  HandleScope scope(thread_);
  ByteArray self(&scope, runtime_->newByteArray());
  const byte byte_array[] = {'f', 'o', 'o'};
  runtime_->byteArrayExtend(thread_, self, byte_array);
  Bytes other(&scope, runtime_->newBytes(1, 'd'));
  Object result(&scope, runBuiltin(METH(bytearray, __add__), self, other));
  EXPECT_TRUE(isByteArrayEqualsCStr(self, "foo"));
  EXPECT_TRUE(isByteArrayEqualsCStr(result, "food"));
}

TEST_F(ByteArrayBuiltinsTest, DunderEqWithNonByteArraySelfRaisesTypeError) {
  EXPECT_TRUE(
      raisedWithStr(runFromCStr(runtime_, "bytearray.__eq__(b'', bytearray())"),
                    LayoutId::kTypeError,
                    "'__eq__' requires a 'bytearray' object but got 'bytes'"));
}

TEST_F(ByteArrayBuiltinsTest, DunderEqWithNonBytesOtherReturnsNotImplemented) {
  HandleScope scope;
  Object self(&scope, runtime_->newByteArray());
  Object other(&scope, SmallInt::fromWord(0));
  Object result(&scope, runBuiltin(METH(bytearray, __eq__), self, other));
  EXPECT_TRUE(result.isNotImplementedType());
}

TEST_F(ByteArrayBuiltinsTest, DunderEqWithEmptyByteArraysReturnsTrue) {
  HandleScope scope;
  Object self(&scope, runtime_->newByteArray());
  Object other(&scope, runtime_->newByteArray());
  EXPECT_EQ(runBuiltin(METH(bytearray, __eq__), self, other), Bool::trueObj());
}

TEST_F(ByteArrayBuiltinsTest, DunderEqWithEqualBytesReturnsTrue) {
  HandleScope scope(thread_);
  const byte bytes[] = {'f', 'o', 'o'};
  ByteArray self(&scope, runtime_->newByteArray());
  runtime_->byteArrayExtend(thread_, self, bytes);
  Object other(&scope, runtime_->newBytesWithAll(bytes));
  EXPECT_EQ(runBuiltin(METH(bytearray, __eq__), self, other), Bool::trueObj());
}

TEST_F(ByteArrayBuiltinsTest, DunderEqWithEqualByteArrayReturnsTrue) {
  HandleScope scope(thread_);
  const byte bytes[] = {'f', 'o', 'o'};
  ByteArray self(&scope, runtime_->newByteArray());
  ByteArray other(&scope, runtime_->newByteArray());
  runtime_->byteArrayExtend(thread_, self, bytes);
  runtime_->byteArrayExtend(thread_, other, bytes);
  EXPECT_EQ(runBuiltin(METH(bytearray, __eq__), self, other), Bool::trueObj());
}

TEST_F(ByteArrayBuiltinsTest, DunderEqWithDifferentLengthsReturnsFalse) {
  HandleScope scope(thread_);
  const byte bytes[] = {'f', 'o', 'o'};
  ByteArray self(&scope, runtime_->newByteArray());
  ByteArray other(&scope, runtime_->newByteArray());
  runtime_->byteArrayExtend(thread_, self, bytes);
  runtime_->byteArrayExtend(thread_, other, {bytes, 2});
  EXPECT_EQ(runBuiltin(METH(bytearray, __eq__), self, other), Bool::falseObj());
}

TEST_F(ByteArrayBuiltinsTest, DunderEqWithDifferentContentsReturnsFalse) {
  HandleScope scope(thread_);
  const byte bytes[] = {'f', 'o', 'o'};
  ByteArray self(&scope, runtime_->newByteArray());
  runtime_->byteArrayExtend(thread_, self, bytes);
  Object other(&scope, runtime_->newBytes(3, 'f'));
  EXPECT_EQ(runBuiltin(METH(bytearray, __eq__), self, other), Bool::falseObj());
}

TEST_F(ByteArrayBuiltinsTest, DunderGeWithNonByteArraySelfRaisesTypeError) {
  EXPECT_TRUE(
      raisedWithStr(runFromCStr(runtime_, "bytearray.__ge__(b'', bytearray())"),
                    LayoutId::kTypeError,
                    "'__ge__' requires a 'bytearray' object but got 'bytes'"));
}

TEST_F(ByteArrayBuiltinsTest, DunderGeWithNonBytesOtherReturnsNotImplemented) {
  HandleScope scope;
  Object self(&scope, runtime_->newByteArray());
  Object other(&scope, SmallInt::fromWord(0));
  Object result(&scope, runBuiltin(METH(bytearray, __ge__), self, other));
  EXPECT_TRUE(result.isNotImplementedType());
}

TEST_F(ByteArrayBuiltinsTest, DunderGeWithEmptyByteArraysReturnsTrue) {
  HandleScope scope;
  Object self(&scope, runtime_->newByteArray());
  Object other(&scope, runtime_->newByteArray());
  EXPECT_EQ(runBuiltin(METH(bytearray, __ge__), self, other), Bool::trueObj());
}

TEST_F(ByteArrayBuiltinsTest, DunderGeithEqualBytesReturnsTrue) {
  HandleScope scope(thread_);
  const byte bytes[] = {'f', 'o', 'o'};
  ByteArray self(&scope, runtime_->newByteArray());
  runtime_->byteArrayExtend(thread_, self, bytes);
  Object other(&scope, runtime_->newBytesWithAll(bytes));
  EXPECT_EQ(runBuiltin(METH(bytearray, __ge__), self, other), Bool::trueObj());
}

TEST_F(ByteArrayBuiltinsTest, DunderGeWithEqualByteArrayReturnsTrue) {
  HandleScope scope(thread_);
  const byte bytes[] = {'f', 'o', 'o'};
  ByteArray self(&scope, runtime_->newByteArray());
  ByteArray other(&scope, runtime_->newByteArray());
  runtime_->byteArrayExtend(thread_, self, bytes);
  runtime_->byteArrayExtend(thread_, other, bytes);
  EXPECT_EQ(runBuiltin(METH(bytearray, __ge__), self, other), Bool::trueObj());
}

TEST_F(ByteArrayBuiltinsTest, DunderGeWithLongerOtherReturnsFalse) {
  HandleScope scope(thread_);
  const byte bytes[] = {'f', 'o', 'o'};
  ByteArray self(&scope, runtime_->newByteArray());
  ByteArray other(&scope, runtime_->newByteArray());
  runtime_->byteArrayExtend(thread_, self, {bytes, 2});
  runtime_->byteArrayExtend(thread_, other, bytes);
  EXPECT_EQ(runBuiltin(METH(bytearray, __ge__), self, other), Bool::falseObj());
}

TEST_F(ByteArrayBuiltinsTest, DunderGeWithShorterOtherReturnsTrue) {
  HandleScope scope(thread_);
  const byte bytes[] = {'f', 'o', 'o'};
  ByteArray self(&scope, runtime_->newByteArray());
  ByteArray other(&scope, runtime_->newByteArray());
  runtime_->byteArrayExtend(thread_, self, bytes);
  runtime_->byteArrayExtend(thread_, other, {bytes, 2});
  EXPECT_EQ(runBuiltin(METH(bytearray, __ge__), self, other), Bool::trueObj());
}

TEST_F(ByteArrayBuiltinsTest, DunderGeWithEarlierOtherReturnsTrue) {
  HandleScope scope(thread_);
  const byte bytes[] = {'f', 'o', 'o'};
  ByteArray self(&scope, runtime_->newByteArray());
  runtime_->byteArrayExtend(thread_, self, bytes);
  Object other(&scope, runtime_->newBytes(3, 'f'));
  EXPECT_EQ(runBuiltin(METH(bytearray, __ge__), self, other), Bool::trueObj());
}

TEST_F(ByteArrayBuiltinsTest, DunderGeWithLaterOtherReturnsFalse) {
  HandleScope scope(thread_);
  const byte bytes[] = {'o', 'o', 'f'};
  ByteArray self(&scope, runtime_->newByteArray());
  runtime_->byteArrayExtend(thread_, self, bytes);
  Object other(&scope, runtime_->newBytes(3, 'o'));
  EXPECT_EQ(runBuiltin(METH(bytearray, __ge__), self, other), Bool::falseObj());
}

TEST_F(ByteArrayBuiltinsTest, DunderGtWithNonByteArraySelfRaisesTypeError) {
  EXPECT_TRUE(
      raisedWithStr(runFromCStr(runtime_, "bytearray.__gt__(b'', bytearray())"),
                    LayoutId::kTypeError,
                    "'__gt__' requires a 'bytearray' object but got 'bytes'"));
}

TEST_F(ByteArrayBuiltinsTest, DunderGtWithNonBytesOtherReturnsNotImplemented) {
  HandleScope scope;
  Object self(&scope, runtime_->newByteArray());
  Object other(&scope, SmallInt::fromWord(0));
  Object result(&scope, runBuiltin(METH(bytearray, __gt__), self, other));
  EXPECT_TRUE(result.isNotImplementedType());
}

TEST_F(ByteArrayBuiltinsTest, DunderGtWithEmptyByteArraysReturnsFalse) {
  HandleScope scope;
  Object self(&scope, runtime_->newByteArray());
  Object other(&scope, runtime_->newByteArray());
  EXPECT_EQ(runBuiltin(METH(bytearray, __gt__), self, other), Bool::falseObj());
}

TEST_F(ByteArrayBuiltinsTest, DunderGtithEqualBytesReturnsFalse) {
  HandleScope scope(thread_);
  const byte bytes[] = {'f', 'o', 'o'};
  ByteArray self(&scope, runtime_->newByteArray());
  runtime_->byteArrayExtend(thread_, self, bytes);
  Object other(&scope, runtime_->newBytesWithAll(bytes));
  EXPECT_EQ(runBuiltin(METH(bytearray, __gt__), self, other), Bool::falseObj());
}

TEST_F(ByteArrayBuiltinsTest, DunderGtWithEqualByteArrayReturnsFalse) {
  HandleScope scope(thread_);
  const byte bytes[] = {'f', 'o', 'o'};
  ByteArray self(&scope, runtime_->newByteArray());
  ByteArray other(&scope, runtime_->newByteArray());
  runtime_->byteArrayExtend(thread_, self, bytes);
  runtime_->byteArrayExtend(thread_, other, bytes);
  EXPECT_EQ(runBuiltin(METH(bytearray, __gt__), self, other), Bool::falseObj());
}

TEST_F(ByteArrayBuiltinsTest, DunderGtWithLongerOtherReturnsFalse) {
  HandleScope scope(thread_);
  const byte bytes[] = {'f', 'o', 'o'};
  ByteArray self(&scope, runtime_->newByteArray());
  ByteArray other(&scope, runtime_->newByteArray());
  runtime_->byteArrayExtend(thread_, self, {bytes, 2});
  runtime_->byteArrayExtend(thread_, other, bytes);
  EXPECT_EQ(runBuiltin(METH(bytearray, __gt__), self, other), Bool::falseObj());
}

TEST_F(ByteArrayBuiltinsTest, DunderGtWithShorterOtherReturnsTrue) {
  HandleScope scope(thread_);
  const byte bytes[] = {'f', 'o', 'o'};
  ByteArray self(&scope, runtime_->newByteArray());
  ByteArray other(&scope, runtime_->newByteArray());
  runtime_->byteArrayExtend(thread_, self, bytes);
  runtime_->byteArrayExtend(thread_, other, {bytes, 2});
  EXPECT_EQ(runBuiltin(METH(bytearray, __gt__), self, other), Bool::trueObj());
}

TEST_F(ByteArrayBuiltinsTest, DunderGtWithEarlierOtherReturnsTrue) {
  HandleScope scope(thread_);
  const byte bytes[] = {'f', 'o', 'o'};
  ByteArray self(&scope, runtime_->newByteArray());
  runtime_->byteArrayExtend(thread_, self, bytes);
  Object other(&scope, runtime_->newBytes(3, 'f'));
  EXPECT_EQ(runBuiltin(METH(bytearray, __gt__), self, other), Bool::trueObj());
}

TEST_F(ByteArrayBuiltinsTest, DunderGtWithLaterOtherReturnsFalse) {
  HandleScope scope(thread_);
  const byte bytes[] = {'o', 'o', 'f'};
  ByteArray self(&scope, runtime_->newByteArray());
  runtime_->byteArrayExtend(thread_, self, bytes);
  Object other(&scope, runtime_->newBytes(3, 'o'));
  EXPECT_EQ(runBuiltin(METH(bytearray, __gt__), self, other), Bool::falseObj());
}

TEST_F(ByteArrayBuiltinsTest, DunderIaddWithNonByteArraySelfRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(runtime_, "bytearray.__iadd__(b'', b'')"),
      LayoutId::kTypeError,
      "'__iadd__' requires a 'bytearray' object but got 'bytes'"));
}

TEST_F(ByteArrayBuiltinsTest, DunderIaddWithNonBytesLikeRaisesTypeError) {
  const char* test = R"(
array = bytearray(b'')
array += None
)";
  EXPECT_TRUE(
      raisedWithStr(runFromCStr(runtime_, test), LayoutId::kTypeError,
                    "can only concatenate bytearray or bytes to bytearray"));
}

TEST_F(ByteArrayBuiltinsTest, DunderIaddWithByteArrayOtherConcatenatesToSelf) {
  HandleScope scope(thread_);
  ByteArray self(&scope, runtime_->newByteArray());
  ByteArray other(&scope, runtime_->newByteArray());
  const byte bytes[] = {'1', '2', '3'};
  runtime_->byteArrayExtend(thread_, other, bytes);
  Object result(&scope, runBuiltin(METH(bytearray, __iadd__), self, other));
  EXPECT_TRUE(isByteArrayEqualsBytes(self, bytes));
  EXPECT_TRUE(isByteArrayEqualsBytes(result, bytes));
}

TEST_F(ByteArrayBuiltinsTest, DunderIaddWithBytesOtherConcatenatesToSelf) {
  HandleScope scope;
  ByteArray self(&scope, runtime_->newByteArray());
  const byte bytes[] = {'1', '2', '3'};
  Bytes other(&scope, runtime_->newBytesWithAll(bytes));
  Object result(&scope, runBuiltin(METH(bytearray, __iadd__), self, other));
  EXPECT_TRUE(isByteArrayEqualsBytes(self, bytes));
  EXPECT_TRUE(isByteArrayEqualsBytes(result, bytes));
}

TEST_F(ByteArrayBuiltinsTest,
       DunderIaddWithBytesSubclassOtherConcatenatesToSelf) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class Foo(bytes): pass
other = Foo(b"1234")
)")
                   .isError());
  HandleScope scope;
  Object self(&scope, runtime_->newByteArray());
  Object other(&scope, mainModuleAt(runtime_, "other"));
  Object result(&scope, runBuiltin(METH(bytearray, __iadd__), self, other));
  const char* expected = "1234";
  EXPECT_TRUE(isByteArrayEqualsCStr(self, expected));
  EXPECT_TRUE(isByteArrayEqualsCStr(result, expected));
}

TEST_F(ByteArrayBuiltinsTest, DunderImulWithNonByteArrayRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(runtime_, "bytearray.__imul__(b'', 1)"), LayoutId::kTypeError,
      "'__imul__' requires a 'bytearray' object but got 'bytes'"));
}

TEST_F(ByteArrayBuiltinsTest, DunderImulWithNonIntRaisesTypeError) {
  HandleScope scope;
  Object self(&scope, runtime_->newByteArray());
  Object count(&scope, runtime_->newList());
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(METH(bytearray, __imul__), self, count), LayoutId::kTypeError,
      "'list' object cannot be interpreted as an integer"));
}

TEST_F(ByteArrayBuiltinsTest, DunderImulWithIntSubclassReturnsRepeatedBytes) {
  HandleScope scope(thread_);
  ByteArray self(&scope, runtime_->newByteArray());
  byteArrayAdd(thread_, runtime_, self, 'a');
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C(int): pass
count = C(5)
)")
                   .isError());
  Object count(&scope, mainModuleAt(runtime_, "count"));
  Object result(&scope, runBuiltin(METH(bytearray, __imul__), self, count));
  EXPECT_TRUE(isByteArrayEqualsCStr(result, "aaaaa"));
}

TEST_F(ByteArrayBuiltinsTest, DunderImulWithDunderIndexReturnsRepeatedBytes) {
  HandleScope scope(thread_);
  ByteArray self(&scope, runtime_->newByteArray());
  byteArrayAdd(thread_, runtime_, self, 'a');
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C:
  def __index__(self):
    return 2
count = C()
)")
                   .isError());
  Object count(&scope, mainModuleAt(runtime_, "count"));
  Object result(&scope, runBuiltin(METH(bytearray, __imul__), self, count));
  EXPECT_TRUE(isByteArrayEqualsCStr(result, "aa"));
}

TEST_F(ByteArrayBuiltinsTest, DunderImulWithBadDunderIndexRaisesTypeError) {
  HandleScope scope;
  Object self(&scope, runtime_->newByteArray());
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C:
  def __index__(self):
    return "foo"
count = C()
)")
                   .isError());
  Object count(&scope, mainModuleAt(runtime_, "count"));
  EXPECT_TRUE(raisedWithStr(runBuiltin(METH(bytearray, __imul__), self, count),
                            LayoutId::kTypeError,
                            "__index__ returned non-int (type str)"));
}

TEST_F(ByteArrayBuiltinsTest, DunderImulPropagatesDunderIndexError) {
  HandleScope scope;
  Object self(&scope, runtime_->newByteArray());
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C:
  def __index__(self):
    raise ArithmeticError("called __index__")
count = C()
)")
                   .isError());
  Object count(&scope, mainModuleAt(runtime_, "count"));
  EXPECT_TRUE(raisedWithStr(runBuiltin(METH(bytearray, __imul__), self, count),
                            LayoutId::kArithmeticError, "called __index__"));
}

TEST_F(ByteArrayBuiltinsTest, DunderImulWithLargeIntRaisesOverflowError) {
  HandleScope scope;
  ByteArray self(&scope, runtime_->newByteArray());
  const uword digits[] = {1, 1};
  Object count(&scope, runtime_->newIntWithDigits(digits));
  EXPECT_TRUE(raisedWithStr(runBuiltin(METH(bytearray, __imul__), self, count),
                            LayoutId::kOverflowError,
                            "cannot fit 'int' into an index-sized integer"));
}

TEST_F(ByteArrayBuiltinsTest, DunderImulWithOverflowRaisesMemoryError) {
  HandleScope scope(thread_);
  ByteArray self(&scope, runtime_->newByteArray());
  const byte bytes[] = {'a', 'b', 'c'};
  runtime_->byteArrayExtend(thread_, self, bytes);
  Object count(&scope, SmallInt::fromWord(SmallInt::kMaxValue / 2));
  EXPECT_TRUE(raised(runBuiltin(METH(bytearray, __imul__), self, count),
                     LayoutId::kMemoryError));
}

TEST_F(ByteArrayBuiltinsTest,
       DunderImulWithEmptyByteArrayReturnsEmptyByteArray) {
  HandleScope scope;
  Object self(&scope, runtime_->newByteArray());
  Object count(&scope, SmallInt::fromWord(5));
  Object result(&scope, runBuiltin(METH(bytearray, __imul__), self, count));
  EXPECT_TRUE(isByteArrayEqualsCStr(result, ""));
}

TEST_F(ByteArrayBuiltinsTest, DunderImulWithNegativeReturnsEmptyByteArray) {
  HandleScope scope;
  ByteArray self(&scope, runtime_->newByteArray());
  self.setBytes(runtime_->mutableBytesWith(8, 'a'));
  self.setNumItems(8);
  Object count(&scope, SmallInt::fromWord(-5));
  Object result(&scope, runBuiltin(METH(bytearray, __imul__), self, count));
  EXPECT_TRUE(isByteArrayEqualsCStr(result, ""));
}

TEST_F(ByteArrayBuiltinsTest, DunderImulWithZeroReturnsEmptyByteArray) {
  HandleScope scope;
  ByteArray self(&scope, runtime_->newByteArray());
  self.setBytes(runtime_->mutableBytesWith(8, 'a'));
  self.setNumItems(8);
  Object count(&scope, SmallInt::fromWord(0));
  Object result(&scope, runBuiltin(METH(bytearray, __imul__), self, count));
  EXPECT_TRUE(isByteArrayEqualsCStr(result, ""));
}

TEST_F(ByteArrayBuiltinsTest, DunderImulWithOneReturnsSameByteArray) {
  HandleScope scope(thread_);
  ByteArray self(&scope, runtime_->newByteArray());
  const byte bytes[] = {'a', 'b'};
  runtime_->byteArrayExtend(thread_, self, bytes);
  Object count(&scope, SmallInt::fromWord(1));
  Object result(&scope, runBuiltin(METH(bytearray, __imul__), self, count));
  EXPECT_TRUE(isByteArrayEqualsBytes(result, bytes));
}

TEST_F(ByteArrayBuiltinsTest, DunderImulReturnsRepeatedByteArray) {
  HandleScope scope(thread_);
  ByteArray self(&scope, runtime_->newByteArray());
  const byte bytes[] = {'a', 'b'};
  runtime_->byteArrayExtend(thread_, self, bytes);
  Object count(&scope, SmallInt::fromWord(3));
  Object result(&scope, runBuiltin(METH(bytearray, __imul__), self, count));
  EXPECT_TRUE(isByteArrayEqualsCStr(result, "ababab"));
}

TEST_F(ByteArrayBuiltinsTest, DunderLeWithNonByteArraySelfRaisesTypeError) {
  EXPECT_TRUE(
      raisedWithStr(runFromCStr(runtime_, "bytearray.__le__(b'', bytearray())"),
                    LayoutId::kTypeError,
                    "'__le__' requires a 'bytearray' object but got 'bytes'"));
}

TEST_F(ByteArrayBuiltinsTest, DunderLeWithNonBytesOtherReturnsNotImplemented) {
  HandleScope scope;
  Object self(&scope, runtime_->newByteArray());
  Object other(&scope, SmallInt::fromWord(0));
  Object result(&scope, runBuiltin(METH(bytearray, __le__), self, other));
  EXPECT_TRUE(result.isNotImplementedType());
}

TEST_F(ByteArrayBuiltinsTest, DunderLeWithEmptyByteArraysReturnsTrue) {
  HandleScope scope;
  Object self(&scope, runtime_->newByteArray());
  Object other(&scope, runtime_->newByteArray());
  EXPECT_EQ(runBuiltin(METH(bytearray, __le__), self, other), Bool::trueObj());
}

TEST_F(ByteArrayBuiltinsTest, DunderLeithEqualBytesReturnsTrue) {
  HandleScope scope(thread_);
  const byte bytes[] = {'f', 'o', 'o'};
  ByteArray self(&scope, runtime_->newByteArray());
  runtime_->byteArrayExtend(thread_, self, bytes);
  Object other(&scope, runtime_->newBytesWithAll(bytes));
  EXPECT_EQ(runBuiltin(METH(bytearray, __le__), self, other), Bool::trueObj());
}

TEST_F(ByteArrayBuiltinsTest, DunderLeWithEqualByteArrayReturnsTrue) {
  HandleScope scope(thread_);
  const byte bytes[] = {'f', 'o', 'o'};
  ByteArray self(&scope, runtime_->newByteArray());
  ByteArray other(&scope, runtime_->newByteArray());
  runtime_->byteArrayExtend(thread_, self, bytes);
  runtime_->byteArrayExtend(thread_, other, bytes);
  EXPECT_EQ(runBuiltin(METH(bytearray, __le__), self, other), Bool::trueObj());
}

TEST_F(ByteArrayBuiltinsTest, DunderLeWithLongerOtherReturnsTrue) {
  HandleScope scope(thread_);
  const byte bytes[] = {'f', 'o', 'o'};
  ByteArray self(&scope, runtime_->newByteArray());
  ByteArray other(&scope, runtime_->newByteArray());
  runtime_->byteArrayExtend(thread_, self, {bytes, 2});
  runtime_->byteArrayExtend(thread_, other, bytes);
  EXPECT_EQ(runBuiltin(METH(bytearray, __le__), self, other), Bool::trueObj());
}

TEST_F(ByteArrayBuiltinsTest, DunderLeWithShorterOtherReturnsFalse) {
  HandleScope scope(thread_);
  const byte bytes[] = {'f', 'o', 'o'};
  ByteArray self(&scope, runtime_->newByteArray());
  ByteArray other(&scope, runtime_->newByteArray());
  runtime_->byteArrayExtend(thread_, self, bytes);
  runtime_->byteArrayExtend(thread_, other, {bytes, 2});
  EXPECT_EQ(runBuiltin(METH(bytearray, __le__), self, other), Bool::falseObj());
}

TEST_F(ByteArrayBuiltinsTest, DunderLeWithEarlierOtherReturnsFalse) {
  HandleScope scope(thread_);
  const byte bytes[] = {'f', 'o', 'o'};
  ByteArray self(&scope, runtime_->newByteArray());
  runtime_->byteArrayExtend(thread_, self, bytes);
  Object other(&scope, runtime_->newBytes(3, 'f'));
  EXPECT_EQ(runBuiltin(METH(bytearray, __le__), self, other), Bool::falseObj());
}

TEST_F(ByteArrayBuiltinsTest, DunderLeWithLaterOtherReturnsTrue) {
  HandleScope scope(thread_);
  const byte bytes[] = {'o', 'o', 'f'};
  ByteArray self(&scope, runtime_->newByteArray());
  runtime_->byteArrayExtend(thread_, self, bytes);
  Object other(&scope, runtime_->newBytes(3, 'o'));
  EXPECT_EQ(runBuiltin(METH(bytearray, __le__), self, other), Bool::trueObj());
}

TEST_F(ByteArrayBuiltinsTest, DunderLenWithNonByteArrayRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(runtime_, "bytearray.__len__(b'')"), LayoutId::kTypeError,
      "'__len__' requires a 'bytearray' object but got 'bytes'"));
}

TEST_F(ByteArrayBuiltinsTest, DunderLenWithEmptyByteArrayReturnsZero) {
  HandleScope scope;
  ByteArray self(&scope, runtime_->newByteArray());
  Object result(&scope, runBuiltin(METH(bytearray, __len__), self));
  EXPECT_TRUE(isIntEqualsWord(*result, 0));
}

TEST_F(ByteArrayBuiltinsTest, DunderLenWithNonEmptyByteArrayReturnsPositive) {
  HandleScope scope(thread_);
  ByteArray self(&scope, runtime_->newByteArray());
  const byte bytes[] = {1, 2, 3, 4, 5};
  runtime_->byteArrayExtend(thread_, self, bytes);
  Object result(&scope, runBuiltin(METH(bytearray, __len__), self));
  EXPECT_TRUE(isIntEqualsWord(*result, 5));

  const byte bytes2[] = {6, 7};
  runtime_->byteArrayExtend(thread_, self, bytes2);
  result = runBuiltin(METH(bytearray, __len__), self);
  EXPECT_TRUE(isIntEqualsWord(*result, 7));
}

TEST_F(ByteArrayBuiltinsTest, DunderLtWithNonByteArraySelfRaisesTypeError) {
  EXPECT_TRUE(
      raisedWithStr(runFromCStr(runtime_, "bytearray.__lt__(b'', bytearray())"),
                    LayoutId::kTypeError,
                    "'__lt__' requires a 'bytearray' object but got 'bytes'"));
}

TEST_F(ByteArrayBuiltinsTest, DunderLtWithNonBytesOtherReturnsNotImplemented) {
  HandleScope scope;
  Object self(&scope, runtime_->newByteArray());
  Object other(&scope, SmallInt::fromWord(0));
  Object result(&scope, runBuiltin(METH(bytearray, __lt__), self, other));
  EXPECT_TRUE(result.isNotImplementedType());
}

TEST_F(ByteArrayBuiltinsTest, DunderLtWithEmptyByteArraysReturnsFalse) {
  HandleScope scope;
  Object self(&scope, runtime_->newByteArray());
  Object other(&scope, runtime_->newByteArray());
  EXPECT_EQ(runBuiltin(METH(bytearray, __lt__), self, other), Bool::falseObj());
}

TEST_F(ByteArrayBuiltinsTest, DunderLtithEqualBytesReturnsFalse) {
  HandleScope scope(thread_);
  const byte bytes[] = {'f', 'o', 'o'};
  ByteArray self(&scope, runtime_->newByteArray());
  runtime_->byteArrayExtend(thread_, self, bytes);
  Object other(&scope, runtime_->newBytesWithAll(bytes));
  EXPECT_EQ(runBuiltin(METH(bytearray, __lt__), self, other), Bool::falseObj());
}

TEST_F(ByteArrayBuiltinsTest, DunderLtWithEqualByteArrayReturnsFalse) {
  HandleScope scope(thread_);
  const byte bytes[] = {'f', 'o', 'o'};
  ByteArray self(&scope, runtime_->newByteArray());
  ByteArray other(&scope, runtime_->newByteArray());
  runtime_->byteArrayExtend(thread_, self, bytes);
  runtime_->byteArrayExtend(thread_, other, bytes);
  EXPECT_EQ(runBuiltin(METH(bytearray, __lt__), self, other), Bool::falseObj());
}

TEST_F(ByteArrayBuiltinsTest, DunderLtWithLongerOtherReturnsTrue) {
  HandleScope scope(thread_);
  const byte bytes[] = {'f', 'o', 'o'};
  ByteArray self(&scope, runtime_->newByteArray());
  ByteArray other(&scope, runtime_->newByteArray());
  runtime_->byteArrayExtend(thread_, self, {bytes, 2});
  runtime_->byteArrayExtend(thread_, other, bytes);
  EXPECT_EQ(runBuiltin(METH(bytearray, __lt__), self, other), Bool::trueObj());
}

TEST_F(ByteArrayBuiltinsTest, DunderLtWithShorterOtherReturnsFalse) {
  HandleScope scope(thread_);
  const byte bytes[] = {'f', 'o', 'o'};
  ByteArray self(&scope, runtime_->newByteArray());
  ByteArray other(&scope, runtime_->newByteArray());
  runtime_->byteArrayExtend(thread_, self, bytes);
  runtime_->byteArrayExtend(thread_, other, {bytes, 2});
  EXPECT_EQ(runBuiltin(METH(bytearray, __lt__), self, other), Bool::falseObj());
}

TEST_F(ByteArrayBuiltinsTest, DunderLtWithEarlierOtherReturnsFalse) {
  HandleScope scope(thread_);
  const byte bytes[] = {'f', 'o', 'o'};
  ByteArray self(&scope, runtime_->newByteArray());
  runtime_->byteArrayExtend(thread_, self, bytes);
  Object other(&scope, runtime_->newBytes(3, 'f'));
  EXPECT_EQ(runBuiltin(METH(bytearray, __lt__), self, other), Bool::falseObj());
}

TEST_F(ByteArrayBuiltinsTest, DunderLtWithLaterOtherReturnsTrue) {
  HandleScope scope(thread_);
  const byte bytes[] = {'o', 'o', 'f'};
  ByteArray self(&scope, runtime_->newByteArray());
  runtime_->byteArrayExtend(thread_, self, bytes);
  Object other(&scope, runtime_->newBytes(3, 'o'));
  EXPECT_EQ(runBuiltin(METH(bytearray, __lt__), self, other), Bool::trueObj());
}

TEST_F(ByteArrayBuiltinsTest, DunderMulWithNonByteArrayRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(runtime_, "bytearray.__mul__(b'', 1)"), LayoutId::kTypeError,
      "'__mul__' requires a 'bytearray' object but got 'bytes'"));
}

TEST_F(ByteArrayBuiltinsTest, DunderMulWithNonIntRaisesTypeError) {
  HandleScope scope;
  Object self(&scope, runtime_->newByteArray());
  Object count(&scope, runtime_->newList());
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(METH(bytearray, __mul__), self, count), LayoutId::kTypeError,
      "'list' object cannot be interpreted as an integer"));
}

TEST_F(ByteArrayBuiltinsTest, DunderMulWithIntSubclassReturnsRepeatedBytes) {
  HandleScope scope(thread_);
  const byte view[] = {'f', 'o', 'o'};
  ByteArray self(&scope, runtime_->newByteArray());
  runtime_->byteArrayExtend(thread_, self, view);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C(int): pass
count = C(3)
)")
                   .isError());
  Object count(&scope, mainModuleAt(runtime_, "count"));
  Object result(&scope, runBuiltin(METH(bytearray, __mul__), self, count));
  EXPECT_TRUE(isByteArrayEqualsCStr(result, "foofoofoo"));
}

TEST_F(ByteArrayBuiltinsTest, DunderMulWithDunderIndexReturnsRepeatedBytes) {
  HandleScope scope(thread_);
  ByteArray self(&scope, runtime_->newByteArray());
  byteArrayAdd(thread_, runtime_, self, 'a');
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C:
  def __index__(self):
    return 2
count = C()
)")
                   .isError());
  Object count(&scope, mainModuleAt(runtime_, "count"));
  Object result(&scope, runBuiltin(METH(bytearray, __mul__), self, count));
  EXPECT_TRUE(isByteArrayEqualsCStr(result, "aa"));
}

TEST_F(ByteArrayBuiltinsTest, DunderMulWithBadDunderIndexRaisesTypeError) {
  HandleScope scope;
  Object self(&scope, runtime_->newByteArray());
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C:
  def __index__(self):
    return "foo"
count = C()
)")
                   .isError());
  Object count(&scope, mainModuleAt(runtime_, "count"));
  EXPECT_TRUE(raisedWithStr(runBuiltin(METH(bytearray, __mul__), self, count),
                            LayoutId::kTypeError,
                            "__index__ returned non-int (type str)"));
}

TEST_F(ByteArrayBuiltinsTest, DunderMulPropagatesDunderIndexError) {
  HandleScope scope;
  Object self(&scope, runtime_->newByteArray());
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C:
  def __index__(self):
    raise ArithmeticError("called __index__")
count = C()
)")
                   .isError());
  Object count(&scope, mainModuleAt(runtime_, "count"));
  EXPECT_TRUE(raisedWithStr(runBuiltin(METH(bytearray, __mul__), self, count),
                            LayoutId::kArithmeticError, "called __index__"));
}

TEST_F(ByteArrayBuiltinsTest, DunderMulWithLargeIntRaisesOverflowError) {
  HandleScope scope;
  ByteArray self(&scope, runtime_->newByteArray());
  const uword digits[] = {1, 1};
  Object count(&scope, runtime_->newIntWithDigits(digits));
  EXPECT_TRUE(raisedWithStr(runBuiltin(METH(bytearray, __mul__), self, count),
                            LayoutId::kOverflowError,
                            "cannot fit 'int' into an index-sized integer"));
}

TEST_F(ByteArrayBuiltinsTest, DunderMulWithOverflowRaisesMemoryError) {
  HandleScope scope(thread_);
  ByteArray self(&scope, runtime_->newByteArray());
  const byte bytes[] = {'a', 'b', 'c'};
  runtime_->byteArrayExtend(thread_, self, bytes);
  Object count(&scope, SmallInt::fromWord(SmallInt::kMaxValue / 2));
  EXPECT_TRUE(raised(runBuiltin(METH(bytearray, __mul__), self, count),
                     LayoutId::kMemoryError));
}

TEST_F(ByteArrayBuiltinsTest,
       DunderMulWithEmptyByteArrayReturnsEmptyByteArray) {
  HandleScope scope;
  Object self(&scope, runtime_->newByteArray());
  Object count(&scope, SmallInt::fromWord(5));
  Object result(&scope, runBuiltin(METH(bytearray, __mul__), self, count));
  EXPECT_TRUE(isByteArrayEqualsCStr(result, ""));
}

TEST_F(ByteArrayBuiltinsTest, DunderMulWithNegativeReturnsEmptyByteArray) {
  HandleScope scope;
  ByteArray self(&scope, runtime_->newByteArray());
  self.setBytes(runtime_->mutableBytesWith(8, 'a'));
  self.setNumItems(8);
  Object count(&scope, SmallInt::fromWord(-5));
  Object result(&scope, runBuiltin(METH(bytearray, __mul__), self, count));
  EXPECT_TRUE(isByteArrayEqualsCStr(result, ""));
}

TEST_F(ByteArrayBuiltinsTest, DunderMulWithZeroReturnsEmptyByteArray) {
  HandleScope scope;
  ByteArray self(&scope, runtime_->newByteArray());
  self.setBytes(runtime_->mutableBytesWith(8, 'a'));
  self.setNumItems(8);
  Object count(&scope, SmallInt::fromWord(0));
  Object result(&scope, runBuiltin(METH(bytearray, __mul__), self, count));
  EXPECT_TRUE(isByteArrayEqualsCStr(result, ""));
}

TEST_F(ByteArrayBuiltinsTest, DunderMulWithOneReturnsSameByteArray) {
  HandleScope scope(thread_);
  ByteArray self(&scope, runtime_->newByteArray());
  const byte bytes[] = {'a', 'b'};
  runtime_->byteArrayExtend(thread_, self, bytes);
  Object count(&scope, SmallInt::fromWord(1));
  Object result(&scope, runBuiltin(METH(bytearray, __mul__), self, count));
  EXPECT_TRUE(isByteArrayEqualsBytes(result, bytes));
}

TEST_F(ByteArrayBuiltinsTest, DunderMulReturnsRepeatedByteArray) {
  HandleScope scope(thread_);
  ByteArray self(&scope, runtime_->newByteArray());
  const byte bytes[] = {'a', 'b'};
  runtime_->byteArrayExtend(thread_, self, bytes);
  Object count(&scope, SmallInt::fromWord(3));
  Object result(&scope, runBuiltin(METH(bytearray, __mul__), self, count));
  EXPECT_TRUE(isByteArrayEqualsCStr(result, "ababab"));
}

TEST_F(ByteArrayBuiltinsTest, DunderNeWithNonByteArraySelfRaisesTypeError) {
  EXPECT_TRUE(
      raisedWithStr(runFromCStr(runtime_, "bytearray.__ne__(b'', bytearray())"),
                    LayoutId::kTypeError,
                    "'__ne__' requires a 'bytearray' object but got 'bytes'"));
}

TEST_F(ByteArrayBuiltinsTest, DunderNeWithNonBytesOtherReturnsNotImplemented) {
  HandleScope scope;
  Object self(&scope, runtime_->newByteArray());
  Object other(&scope, SmallInt::fromWord(0));
  Object result(&scope, runBuiltin(METH(bytearray, __ne__), self, other));
  EXPECT_TRUE(result.isNotImplementedType());
}

TEST_F(ByteArrayBuiltinsTest, DunderNeWithEmptyByteArraysReturnsFalse) {
  HandleScope scope;
  Object self(&scope, runtime_->newByteArray());
  Object other(&scope, runtime_->newByteArray());
  EXPECT_EQ(runBuiltin(METH(bytearray, __ne__), self, other), Bool::falseObj());
}

TEST_F(ByteArrayBuiltinsTest, DunderNeWithEqualBytesReturnsFalse) {
  HandleScope scope(thread_);
  const byte bytes[] = {'f', 'o', 'o'};
  ByteArray self(&scope, runtime_->newByteArray());
  runtime_->byteArrayExtend(thread_, self, bytes);
  Object other(&scope, runtime_->newBytesWithAll(bytes));
  EXPECT_EQ(runBuiltin(METH(bytearray, __ne__), self, other), Bool::falseObj());
}

TEST_F(ByteArrayBuiltinsTest, DunderNeWithEqualByteArrayReturnsFalse) {
  HandleScope scope(thread_);
  const byte bytes[] = {'f', 'o', 'o'};
  ByteArray self(&scope, runtime_->newByteArray());
  ByteArray other(&scope, runtime_->newByteArray());
  runtime_->byteArrayExtend(thread_, self, bytes);
  runtime_->byteArrayExtend(thread_, other, bytes);
  EXPECT_EQ(runBuiltin(METH(bytearray, __ne__), self, other), Bool::falseObj());
}

TEST_F(ByteArrayBuiltinsTest, DunderNeWithDifferentLengthsReturnsTrue) {
  HandleScope scope(thread_);
  const byte bytes[] = {'f', 'o', 'o'};
  ByteArray self(&scope, runtime_->newByteArray());
  ByteArray other(&scope, runtime_->newByteArray());
  runtime_->byteArrayExtend(thread_, self, bytes);
  runtime_->byteArrayExtend(thread_, other, {bytes, 2});
  EXPECT_EQ(runBuiltin(METH(bytearray, __ne__), self, other), Bool::trueObj());
}

TEST_F(ByteArrayBuiltinsTest, DunderNeWithDifferentContentsReturnsTrue) {
  HandleScope scope(thread_);
  const byte bytes[] = {'f', 'o', 'o'};
  ByteArray self(&scope, runtime_->newByteArray());
  runtime_->byteArrayExtend(thread_, self, bytes);
  Object other(&scope, runtime_->newBytes(3, 'f'));
  EXPECT_EQ(runBuiltin(METH(bytearray, __ne__), self, other), Bool::trueObj());
}

TEST_F(ByteArrayBuiltinsTest, DunderNewWithNonTypeRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, "bytearray.__new__(3)"),
                            LayoutId::kTypeError, "not a type object"));
}

TEST_F(ByteArrayBuiltinsTest, DunderNewWithNonByteArrayRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, "bytearray.__new__(int)"),
                            LayoutId::kTypeError,
                            "not a subtype of bytearray"));
}

TEST_F(ByteArrayBuiltinsTest, DunderNewReturnsEmptyByteArray) {
  HandleScope scope;
  Type cls(&scope, runtime_->typeAt(LayoutId::kByteArray));
  Object self(&scope, runBuiltin(METH(bytearray, __new__), cls));
  EXPECT_TRUE(isByteArrayEqualsCStr(self, ""));
}

TEST_F(ByteArrayBuiltinsTest, NewByteArray) {
  HandleScope scope(thread_);
  ASSERT_FALSE(
      runFromCStr(runtime_, "obj = bytearray(b'Hello world!')").isError());
  ByteArray self(&scope, mainModuleAt(runtime_, "obj"));
  EXPECT_TRUE(isByteArrayEqualsCStr(self, "Hello world!"));
}

TEST_F(ByteArrayBuiltinsTest, DunderReprWithNonByteArrayRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(runtime_, "bytearray.__repr__(b'')"), LayoutId::kTypeError,
      "'__repr__' requires a 'bytearray' object but got 'bytes'"));
}

TEST_F(ByteArrayBuiltinsTest, DunderReprWithEmptyByteArrayReturnsEmptyRepr) {
  HandleScope scope;
  ByteArray self(&scope, runtime_->newByteArray());
  Object repr(&scope, runBuiltin(METH(bytearray, __repr__), self));
  EXPECT_TRUE(isStrEqualsCStr(*repr, "bytearray(b'')"));
}

TEST_F(ByteArrayBuiltinsTest, DunderReprWithSimpleByteArrayReturnsRepr) {
  HandleScope scope(thread_);
  ByteArray self(&scope, runtime_->newByteArray());
  const byte bytes[] = {'f', 'o', 'o'};
  runtime_->byteArrayExtend(thread_, self, bytes);
  Object repr(&scope, runBuiltin(METH(bytearray, __repr__), self));
  EXPECT_TRUE(isStrEqualsCStr(*repr, "bytearray(b'foo')"));
}

TEST_F(ByteArrayBuiltinsTest,
       DunderReprWithDoubleQuoteUsesSingleQuoteDelimiters) {
  HandleScope scope(thread_);
  ByteArray self(&scope, runtime_->newByteArray());
  const byte bytes[] = {'_', '"', '_'};
  runtime_->byteArrayExtend(thread_, self, bytes);
  Object repr(&scope, runBuiltin(METH(bytearray, __repr__), self));
  EXPECT_TRUE(isStrEqualsCStr(*repr, R"(bytearray(b'_"_'))"));
}

TEST_F(ByteArrayBuiltinsTest,
       DunderReprWithSingleQuoteUsesDoubleQuoteDelimiters) {
  HandleScope scope(thread_);
  ByteArray self(&scope, runtime_->newByteArray());
  const byte bytes[] = {'_', '\'', '_'};
  runtime_->byteArrayExtend(thread_, self, bytes);
  Object repr(&scope, runBuiltin(METH(bytearray, __repr__), self));
  EXPECT_TRUE(isStrEqualsCStr(*repr, R"(bytearray(b"_\'_"))"));
}

TEST_F(ByteArrayBuiltinsTest,
       DunderReprWithBothQuotesUsesSingleQuoteDelimiters) {
  HandleScope scope(thread_);
  ByteArray self(&scope, runtime_->newByteArray());
  const byte bytes[] = {'_', '"', '_', '\'', '_'};
  runtime_->byteArrayExtend(thread_, self, bytes);
  Object repr(&scope, runBuiltin(METH(bytearray, __repr__), self));
  EXPECT_TRUE(isStrEqualsCStr(*repr, R"(bytearray(b'_"_\'_'))"));
}

TEST_F(ByteArrayBuiltinsTest, DunderReprWithSpeciaBytesUsesEscapeSequences) {
  HandleScope scope(thread_);
  ByteArray self(&scope, runtime_->newByteArray());
  const byte bytes[] = {'\\', '\t', '\n', '\r'};
  runtime_->byteArrayExtend(thread_, self, bytes);
  Object repr(&scope, runBuiltin(METH(bytearray, __repr__), self));
  EXPECT_TRUE(isStrEqualsCStr(*repr, R"(bytearray(b'\\\t\n\r'))"));
}

TEST_F(ByteArrayBuiltinsTest, DunderReprWithSmallAndLargeBytesUsesHex) {
  HandleScope scope(thread_);
  ByteArray self(&scope, runtime_->newByteArray());
  const byte bytes[] = {0, 0x1f, 0x80, 0xff};
  runtime_->byteArrayExtend(thread_, self, bytes);
  Object repr(&scope, runBuiltin(METH(bytearray, __repr__), self));
  EXPECT_TRUE(isStrEqualsCStr(*repr, R"(bytearray(b'\x00\x1f\x80\xff'))"));
}

TEST_F(ByteArrayBuiltinsTest, DunderRmulCallsDunderMul) {
  HandleScope scope;
  ASSERT_FALSE(
      runFromCStr(runtime_, "result = 3 * bytearray(b'123')").isError());
  Object result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_TRUE(isByteArrayEqualsCStr(result, "123123123"));
}

TEST_F(ByteArrayBuiltinsTest, HexWithNonByteArrayRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(runtime_, "bytearray.hex(b'')"), LayoutId::kTypeError,
      "'hex' requires a 'bytearray' object but got 'bytes'"));
}

TEST_F(ByteArrayBuiltinsTest, HexWithEmptyByteArrayReturnsEmptyString) {
  HandleScope scope;
  Object self(&scope, runtime_->newByteArray());
  Object result(&scope, runBuiltin(METH(bytearray, hex), self));
  EXPECT_TRUE(isStrEqualsCStr(*result, ""));
}

TEST_F(ByteArrayBuiltinsTest, HexWithNonEmptyBytesReturnsString) {
  HandleScope scope(thread_);
  ByteArray self(&scope, runtime_->newByteArray());
  const byte bytes[] = {0x60, 0xe, 0x18, 0x21};
  runtime_->byteArrayExtend(thread_, self, bytes);
  Object result(&scope, runBuiltin(METH(bytearray, hex), self));
  EXPECT_TRUE(isStrEqualsCStr(*result, "600e1821"));
}

TEST_F(ByteArrayBuiltinsTest, JoinWithNonIterableRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, "bytearray(b'').join(0)"),
                            LayoutId::kTypeError,
                            "'int' object is not iterable"));
}

TEST_F(ByteArrayBuiltinsTest, JoinWithMistypedIterableRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(runtime_, "bytearray(b' ').join([1])"), LayoutId::kTypeError,
      "sequence item 0: expected a bytes-like object, 'int' found"));
}

TEST_F(ByteArrayBuiltinsTest, JoinWithIterableReturnsByteArray) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class Foo:
  def __iter__(self):
    return [b'ab', b'c', b'def'].__iter__()
result = bytearray(b' ').join(Foo())
)")
                   .isError());
  HandleScope scope(thread_);
  Object result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_TRUE(isByteArrayEqualsCStr(result, "ab c def"));
}

TEST_F(ByteArrayBuiltinsTest, MaketransWithNonBytesLikeFromRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(runtime_, "bytearray.maketrans([1,2], b'ab')"),
      LayoutId::kTypeError, "a bytes-like object is required, not 'list'"));
}

TEST_F(ByteArrayBuiltinsTest, MaketransWithNonBytesLikeToRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(runtime_, "bytearray.maketrans(b'1', 2)"),
      LayoutId::kTypeError, "a bytes-like object is required, not 'int'"));
}

TEST_F(ByteArrayBuiltinsTest, MaketransWithDifferentLengthsRaisesValueError) {
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(runtime_, "bytearray.maketrans(b'12', bytearray())"),
      LayoutId::kValueError, "maketrans arguments must have same length"));
}

TEST_F(ByteArrayBuiltinsTest, MaketransWithEmptyReturnsDefaultBytes) {
  HandleScope scope;
  ASSERT_FALSE(
      runFromCStr(runtime_, "result = bytearray.maketrans(bytearray(), b'')")
          .isError());
  Object result(&scope, mainModuleAt(runtime_, "result"));
  byte expected[256];
  for (word i = 0; i < 256; i++) {
    expected[i] = i;
  }
  EXPECT_TRUE(isBytesEqualsBytes(result, expected));
}

TEST_F(ByteArrayBuiltinsTest, MaketransWithNonEmptyReturnsBytes) {
  HandleScope scope;
  ASSERT_FALSE(
      runFromCStr(runtime_,
                  "result = bytearray.maketrans(bytearray(b'abc'), b'123')")
          .isError());
  Object result(&scope, mainModuleAt(runtime_, "result"));
  ASSERT_TRUE(result.isBytes());
  Bytes actual(&scope, *result);
  EXPECT_EQ(actual.byteAt('a'), '1');
  EXPECT_EQ(actual.byteAt('b'), '2');
  EXPECT_EQ(actual.byteAt('c'), '3');
}

TEST_F(ByteArrayBuiltinsTest, TranslateWithNonByteArraySelfRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(runtime_, "bytearray.translate(b'', None)"),
      LayoutId::kTypeError,
      "'translate' requires a 'bytearray' object but got 'bytes'"));
}

TEST_F(ByteArrayBuiltinsTest, TranslateWithNonBytesLikeTableRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, "bytearray().translate(42)"),
                            LayoutId::kTypeError,
                            "a bytes-like object is required, not 'int'"));
}

TEST_F(ByteArrayBuiltinsTest, TranslateWithNonBytesLikeDeleteRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(runtime_, "bytearray().translate(None, 42)"),
      LayoutId::kTypeError, "a bytes-like object is required, not 'int'"));
}

TEST_F(ByteArrayBuiltinsTest, TranslateWithShortTableRaisesValueError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, "bytearray().translate(b'')"),
                            LayoutId::kValueError,
                            "translation table must be 256 characters long"));
}

TEST_F(ByteArrayBuiltinsTest, TranslateWithEmptyByteArrayReturnsNewByteArray) {
  HandleScope scope;
  Object self(&scope, runtime_->newByteArray());
  Object table(&scope, NoneType::object());
  Object del(&scope, runtime_->newByteArray());
  Object result(&scope,
                runBuiltin(METH(bytearray, translate), self, table, del));
  EXPECT_TRUE(isByteArrayEqualsCStr(result, ""));
  EXPECT_NE(result, self);
}

TEST_F(ByteArrayBuiltinsTest, TranslateWithNonEmptySecondArgDeletesBytes) {
  HandleScope scope(thread_);
  const byte alabama[] = {'A', 'l', 'a', 'b', 'a', 'm', 'a'};
  const byte abc[] = {'a', 'b', 'c'};
  ByteArray self(&scope, runtime_->newByteArray());
  runtime_->byteArrayExtend(thread_, self, alabama);
  Object table(&scope, NoneType::object());
  Object del(&scope, runtime_->newBytesWithAll(abc));
  Object result(&scope,
                runBuiltin(METH(bytearray, translate), self, table, del));
  EXPECT_TRUE(isByteArrayEqualsCStr(result, "Alm"));
}

TEST_F(ByteArrayBuiltinsTest, TranslateWithTableTranslatesBytes) {
  HandleScope scope(thread_);
  ASSERT_FALSE(
      runFromCStr(runtime_, "table = bytes.maketrans(b'Aa', b'12')").isError());
  const byte alabama[] = {'A', 'l', 'a', 'b', 'a', 'm', 'a'};
  ByteArray self(&scope, runtime_->newByteArray());
  runtime_->byteArrayExtend(thread_, self, alabama);
  Object table(&scope, mainModuleAt(runtime_, "table"));
  Object del(&scope, runtime_->newByteArray());
  Object result(&scope,
                runBuiltin(METH(bytearray, translate), self, table, del));
  EXPECT_TRUE(isByteArrayEqualsCStr(result, "1l2b2m2"));
}

TEST_F(ByteArrayBuiltinsTest,
       TranslateWithTableAndDeleteTranslatesAndDeletesBytes) {
  HandleScope scope(thread_);
  ASSERT_FALSE(
      runFromCStr(runtime_, "table = bytes.maketrans(b'Aa', b'12')").isError());
  const byte alabama[] = {'A', 'l', 'a', 'b', 'a', 'm', 'a'};
  const byte abc[] = {'a', 'b', 'c'};
  ByteArray self(&scope, runtime_->newByteArray());
  runtime_->byteArrayExtend(thread_, self, alabama);
  Object table(&scope, mainModuleAt(runtime_, "table"));
  Object del(&scope, runtime_->newBytesWithAll(abc));
  Object result(&scope,
                runBuiltin(METH(bytearray, translate), self, table, del));
  EXPECT_TRUE(isByteArrayEqualsCStr(result, "1lm"));
}

TEST_F(ByteArrayBuiltinsTest, TranslateDeletesAllBytes) {
  HandleScope scope(thread_);
  const byte alabama[] = {'b', 'a', 'c', 'a', 'a', 'c', 'a'};
  const byte abc[] = {'a', 'b', 'c'};
  ByteArray self(&scope, runtime_->newByteArray());
  runtime_->byteArrayExtend(thread_, self, alabama);
  Object table(&scope, NoneType::object());
  Object del(&scope, runtime_->newBytesWithAll(abc));
  Object result(&scope,
                runBuiltin(METH(bytearray, translate), self, table, del));
  EXPECT_TRUE(isByteArrayEqualsCStr(result, ""));
}

TEST_F(ByteArrayBuiltinsTest, DunderIterReturnsByteArrayIterator) {
  ASSERT_FALSE(
      runFromCStr(runtime_, "result = type(bytearray().__iter__())").isError());
  HandleScope scope;
  Object result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_EQ(*result, runtime_->typeAt(LayoutId::kByteArrayIterator));
}

TEST_F(ByteArrayBuiltinsTest, IteratorDunderNextReturnsNextElement) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
ba = bytearray(b'abc')
it = iter(ba)
r0 = it.__next__()
r1 = it.__next__()
r2 = it.__next__()
try:
  it.__next__()
  r3 = False
except StopIteration:
  r3 = True
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(runtime_, "r0"), 'a'));
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(runtime_, "r1"), 'b'));
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(runtime_, "r2"), 'c'));
  EXPECT_EQ(mainModuleAt(runtime_, "r3"), Bool::trueObj());
}

TEST_F(ByteArrayBuiltinsTest,
       IteratorDunderNextStopsIterationWhenByteArrayIsShrunk) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
ba = bytearray(b'abc')
it = iter(ba)
)")
                   .isError());
  HandleScope scope(thread_);
  ByteArray ba(&scope, mainModuleAt(runtime_, "ba"));
  ByteArrayIterator it(&scope, mainModuleAt(runtime_, "it"));
  ba.setNumItems(0);
  EXPECT_TRUE(raised(thread_->invokeMethod1(it, ID(__next__)),
                     LayoutId::kStopIteration));
}

}  // namespace py

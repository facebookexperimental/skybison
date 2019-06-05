#include "gtest/gtest.h"

#include "runtime.h"
#include "slice-builtins.h"
#include "test-utils.h"

namespace python {

using namespace testing;

using SliceBuiltinsTest = RuntimeFixture;

TEST_F(SliceBuiltinsTest, UnpackWithAllNoneSetsDefaults) {
  HandleScope scope(thread_);
  Slice slice(&scope, runtime_.newSlice());
  word start, stop, step;
  Object result(&scope, sliceUnpack(thread_, slice, &start, &stop, &step));
  ASSERT_FALSE(result.isError());
  EXPECT_EQ(start, 0);
  EXPECT_EQ(stop, SmallInt::kMaxValue);
  EXPECT_EQ(step, 1);
}

TEST_F(SliceBuiltinsTest, UnpackWithNegativeStepSetsReverseDefaults) {
  HandleScope scope(thread_);
  Slice slice(&scope, runtime_.newSlice());
  slice.setStep(SmallInt::fromWord(-1));
  word start, stop, step;
  Object result(&scope, sliceUnpack(thread_, slice, &start, &stop, &step));
  ASSERT_FALSE(result.isError());
  EXPECT_EQ(start, SmallInt::kMaxValue);
  EXPECT_EQ(stop, SmallInt::kMinValue);
  EXPECT_EQ(step, -1);
}

TEST_F(SliceBuiltinsTest, UnpackWithNonIndexStartRaisesTypeError) {
  HandleScope scope(thread_);
  Slice slice(&scope, runtime_.newSlice());
  slice.setStart(runtime_.newSet());
  word start, stop, step;
  Object result(&scope, sliceUnpack(thread_, slice, &start, &stop, &step));
  EXPECT_TRUE(raisedWithStr(
      *result, LayoutId::kTypeError,
      "slice indices must be integers or None or have an __index__ method"));
}

TEST_F(SliceBuiltinsTest, UnpackWithNonIndexStopRaisesTypeError) {
  HandleScope scope(thread_);
  Slice slice(&scope, runtime_.newSlice());
  slice.setStop(runtime_.newSet());
  word start, stop, step;
  Object result(&scope, sliceUnpack(thread_, slice, &start, &stop, &step));
  EXPECT_TRUE(raisedWithStr(
      *result, LayoutId::kTypeError,
      "slice indices must be integers or None or have an __index__ method"));
}

TEST_F(SliceBuiltinsTest, UnpackWithNonIndexStepRaisesTypeError) {
  HandleScope scope(thread_);
  Slice slice(&scope, runtime_.newSlice());
  slice.setStep(runtime_.newSet());
  word start, stop, step;
  Object result(&scope, sliceUnpack(thread_, slice, &start, &stop, &step));
  EXPECT_TRUE(raisedWithStr(
      *result, LayoutId::kTypeError,
      "slice indices must be integers or None or have an __index__ method"));
}

TEST_F(SliceBuiltinsTest, UnpackWithMistypedDunderIndexRaisesTypeError) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class Foo:
  def __index__(self): return ""
foo = Foo()
)")
                   .isError());
  HandleScope scope(thread_);
  Slice slice(&scope, runtime_.newSlice());
  slice.setStep(moduleAt(&runtime_, "__main__", "foo"));
  word start, stop, step;
  Object result(&scope, sliceUnpack(thread_, slice, &start, &stop, &step));
  EXPECT_TRUE(raisedWithStr(*result, LayoutId::kTypeError,
                            "__index__ returned non-int (type str)"));
}

TEST_F(SliceBuiltinsTest, UnpackWithNonIntIndicesCallsDunderIndex) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class Foo:
  def __init__(self):
    self.count = 0
  def __index__(self):
    self.count += 1
    return self.count
foo = Foo()
)")
                   .isError());
  HandleScope scope(thread_);
  Object foo(&scope, moduleAt(&runtime_, "__main__", "foo"));
  Slice slice(&scope, runtime_.newSlice());
  slice.setStart(*foo);
  slice.setStop(*foo);
  slice.setStep(*foo);
  word start, stop, step;
  Object result(&scope, sliceUnpack(thread_, slice, &start, &stop, &step));
  ASSERT_FALSE(result.isError());
  EXPECT_EQ(start, 2);
  EXPECT_EQ(stop, 3);
  EXPECT_EQ(step, 1);
}

TEST_F(SliceBuiltinsTest, UnpackWithZeroStepRaisesValueError) {
  HandleScope scope(thread_);
  Slice slice(&scope, runtime_.newSlice());
  slice.setStep(SmallInt::fromWord(0));
  word start, stop, step;
  Object result(&scope, sliceUnpack(thread_, slice, &start, &stop, &step));
  EXPECT_TRUE(raisedWithStr(*result, LayoutId::kValueError,
                            "slice step cannot be zero"));
}

TEST_F(SliceBuiltinsTest, UnpackWithOverflowSilentlyReducesValues) {
  HandleScope scope(thread_);
  Slice slice(&scope, runtime_.newSlice());
  Object large(&scope, runtime_.newInt(SmallInt::kMaxValue + 1));
  slice.setStart(*large);
  slice.setStop(*large);
  slice.setStep(*large);
  word start, stop, step;
  Object result(&scope, sliceUnpack(thread_, slice, &start, &stop, &step));
  ASSERT_FALSE(result.isError());
  EXPECT_EQ(start, SmallInt::kMaxValue);
  EXPECT_EQ(stop, SmallInt::kMaxValue);
  EXPECT_EQ(step, SmallInt::kMaxValue);
}

TEST_F(SliceBuiltinsTest, UnpackWithUnderflowSilentlyBoostsValues) {
  HandleScope scope(thread_);
  Slice slice(&scope, runtime_.newSlice());
  Object large(&scope, runtime_.newInt(SmallInt::kMinValue - 1));
  slice.setStart(*large);
  slice.setStop(*large);
  slice.setStep(*large);
  word start, stop, step;
  Object result(&scope, sliceUnpack(thread_, slice, &start, &stop, &step));
  ASSERT_FALSE(result.isError());
  EXPECT_EQ(start, SmallInt::kMinValue);
  EXPECT_EQ(stop, SmallInt::kMinValue);
  EXPECT_EQ(step, -SmallInt::kMaxValue);
}

TEST_F(SliceBuiltinsTest, SliceHasStartAttribute) {
  HandleScope scope(thread_);
  Layout layout(&scope, runtime_.layoutAt(LayoutId::kSlice));
  Str name(&scope, runtime_.newStrFromCStr("start"));
  AttributeInfo info;
  ASSERT_TRUE(
      runtime_.layoutFindAttribute(Thread::current(), layout, name, &info));
  EXPECT_TRUE(info.isInObject());
  EXPECT_TRUE(info.isFixedOffset());
}

TEST_F(SliceBuiltinsTest, SliceHasStopAttribute) {
  HandleScope scope(thread_);
  Layout layout(&scope, runtime_.layoutAt(LayoutId::kSlice));
  Str name(&scope, runtime_.newStrFromCStr("stop"));
  AttributeInfo info;
  ASSERT_TRUE(
      runtime_.layoutFindAttribute(Thread::current(), layout, name, &info));
  EXPECT_TRUE(info.isInObject());
  EXPECT_TRUE(info.isFixedOffset());
}

TEST_F(SliceBuiltinsTest, SliceHasStepAttribute) {
  HandleScope scope(thread_);
  Layout layout(&scope, runtime_.layoutAt(LayoutId::kSlice));
  Str name(&scope, runtime_.newStrFromCStr("step"));
  AttributeInfo info;
  ASSERT_TRUE(
      runtime_.layoutFindAttribute(Thread::current(), layout, name, &info));
  EXPECT_TRUE(info.isInObject());
  EXPECT_TRUE(info.isFixedOffset());
}

TEST_F(SliceBuiltinsTest, DunderNewWithNonTypeRaisesTypeError) {
  HandleScope scope(thread_);
  Object num(&scope, SmallInt::fromWord(0));
  Object result(&scope, runBuiltin(SliceBuiltins::dunderNew, num));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

TEST_F(SliceBuiltinsTest, DunderNewWithNonSliceTypeRaisesTypeError) {
  HandleScope scope(thread_);
  Object type(&scope, runtime_.typeAt(LayoutId::kInt));
  Object result(&scope, runBuiltin(SliceBuiltins::dunderNew, type));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

TEST_F(SliceBuiltinsTest, DunderNewWithOneArgSetsStop) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, "result = slice(0)").isError());
  Object result(&scope, moduleAt(&runtime_, "__main__", "result"));
  ASSERT_TRUE(result.isSlice());
  Slice slice(&scope, *result);
  EXPECT_EQ(slice.start(), NoneType::object());
  EXPECT_EQ(slice.stop(), SmallInt::fromWord(0));
  EXPECT_EQ(slice.step(), NoneType::object());
}

TEST_F(SliceBuiltinsTest, DunderNewWithTwoArgsSetsStartAndStop) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, "result = slice(0, 1)").isError());
  Object result(&scope, moduleAt(&runtime_, "__main__", "result"));
  ASSERT_TRUE(result.isSlice());
  Slice slice(&scope, *result);
  EXPECT_EQ(slice.start(), SmallInt::fromWord(0));
  EXPECT_EQ(slice.stop(), SmallInt::fromWord(1));
  EXPECT_EQ(slice.step(), NoneType::object());
}

TEST_F(SliceBuiltinsTest, DunderNewWithThreeArgsSetsAllIndices) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, "result = slice(0, 1, 2)").isError());
  Object result(&scope, moduleAt(&runtime_, "__main__", "result"));
  ASSERT_TRUE(result.isSlice());
  Slice slice(&scope, *result);
  EXPECT_EQ(slice.start(), SmallInt::fromWord(0));
  EXPECT_EQ(slice.stop(), SmallInt::fromWord(1));
  EXPECT_EQ(slice.step(), SmallInt::fromWord(2));
}

TEST_F(SliceBuiltinsTest, IndicesWithNonSliceRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime_, "slice.indices([], 1)"), LayoutId::kTypeError,
      "'indices' requires a 'slice' object but received a 'list'"));
}

TEST_F(SliceBuiltinsTest, IndicesWithNonIntRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime_, "slice(1).indices([])"), LayoutId::kTypeError,
      "'list' object cannot be interpreted as an integer"));
}

TEST_F(SliceBuiltinsTest, IndicesWithNegativeLengthRaisesValueError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, "slice(1).indices(-1)"),
                            LayoutId::kValueError,
                            "length should not be negative"));
}

TEST_F(SliceBuiltinsTest, IndicesWithZeroStepRaisesValueError) {
  EXPECT_TRUE(
      raisedWithStr(runFromCStr(&runtime_, "slice(1, 1, 0).indices(10)"),
                    LayoutId::kValueError, "slice step cannot be zero"));
}

TEST_F(SliceBuiltinsTest, IndicesWithNonIntStartRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime_, "slice('').indices(10)"), LayoutId::kTypeError,
      "slice indices must be integers or None or have an __index__ method"));
}

TEST_F(SliceBuiltinsTest, IndicesWithNonIntStopRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime_, "slice(1, '').indices(10)"), LayoutId::kTypeError,
      "slice indices must be integers or None or have an __index__ method"));
}

TEST_F(SliceBuiltinsTest, IndicesWithNonIntStepRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime_, "slice(1, 6, '').indices(10)"),
      LayoutId::kTypeError,
      "slice indices must be integers or None or have an __index__ method"));
}

TEST_F(SliceBuiltinsTest, IndicesWithNoneReturnsDefaults) {
  HandleScope scope(thread_);
  ASSERT_FALSE(
      runFromCStr(&runtime_, "result = slice(None).indices(10)").isError());
  Object result(&scope, moduleAt(&runtime_, "__main__", "result"));
  ASSERT_TRUE(result.isTuple());
  Tuple indices(&scope, *result);
  ASSERT_EQ(indices.length(), 3);
  EXPECT_EQ(indices.at(0), SmallInt::fromWord(0));
  EXPECT_EQ(indices.at(1), SmallInt::fromWord(10));
  EXPECT_EQ(indices.at(2), SmallInt::fromWord(1));
}

TEST_F(SliceBuiltinsTest, IndicesWithNoneAndNegativeReturnsDefaults) {
  HandleScope scope(thread_);
  ASSERT_FALSE(
      runFromCStr(&runtime_, "result = slice(None, None, -1).indices(10)")
          .isError());
  Object result(&scope, moduleAt(&runtime_, "__main__", "result"));
  ASSERT_TRUE(result.isTuple());
  Tuple indices(&scope, *result);
  ASSERT_EQ(indices.length(), 3);
  EXPECT_EQ(indices.at(0), SmallInt::fromWord(9));
  EXPECT_EQ(indices.at(1), SmallInt::fromWord(-1));
  EXPECT_EQ(indices.at(2), SmallInt::fromWord(-1));
}

TEST_F(SliceBuiltinsTest, IndicesCallsDunderIndex) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class Idx:
  def __init__(self):
    self.count = 0
  def __index__(self):
    self.count += 1
    return self.count
idx = Idx()
result = slice(idx, idx, idx).indices(10)
)")
                   .isError());
  Object result(&scope, moduleAt(&runtime_, "__main__", "result"));
  ASSERT_TRUE(result.isTuple());
  Tuple indices(&scope, *result);
  ASSERT_EQ(indices.length(), 3);
  EXPECT_EQ(indices.at(0), SmallInt::fromWord(2));
  EXPECT_EQ(indices.at(1), SmallInt::fromWord(3));
  EXPECT_EQ(indices.at(2), SmallInt::fromWord(1));
}

TEST_F(SliceBuiltinsTest, IndicesTruncatesToLength) {
  HandleScope scope(thread_);
  ASSERT_FALSE(
      runFromCStr(&runtime_, "result = slice(-4, 10, 2).indices(5)").isError());
  Object result(&scope, moduleAt(&runtime_, "__main__", "result"));
  ASSERT_TRUE(result.isTuple());
  Tuple indices(&scope, *result);
  ASSERT_EQ(indices.length(), 3);
  EXPECT_EQ(indices.at(0), SmallInt::fromWord(1));
  EXPECT_EQ(indices.at(1), SmallInt::fromWord(5));
  EXPECT_EQ(indices.at(2), SmallInt::fromWord(2));
}

}  // namespace python

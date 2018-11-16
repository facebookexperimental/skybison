#include "gtest/gtest.h"

#include <cmath>

#include "frame.h"
#include "handles.h"
#include "int-builtins.h"
#include "objects.h"
#include "runtime.h"
#include "test-utils.h"

namespace python {

using namespace testing;

TEST(IntBuiltinsTest, NewWithStringReturnsInt) {
  Runtime runtime;
  runtime.runFromCStr(R"(
a = int("123")
b = int("-987")
)");
  HandleScope scope;
  Module main(&scope, findModule(&runtime, "__main__"));
  Int a(&scope, moduleAt(&runtime, main, "a"));
  Int b(&scope, moduleAt(&runtime, main, "b"));
  EXPECT_EQ(a->asWord(), 123);
  EXPECT_EQ(b->asWord(), -987);
}

TEST(IntBuiltinsTest, NewWithStringAndIntBaseReturnsInt) {
  Runtime runtime;
  runtime.runFromCStr(R"(
a = int("23", 8)
b = int("abc", 16)
c = int("023", 0)
d = int("0xabc", 0)
)");
  HandleScope scope;
  Module main(&scope, findModule(&runtime, "__main__"));
  Int a(&scope, moduleAt(&runtime, main, "a"));
  Int b(&scope, moduleAt(&runtime, main, "b"));
  Int c(&scope, moduleAt(&runtime, main, "c"));
  Int d(&scope, moduleAt(&runtime, main, "d"));
  EXPECT_EQ(a->asWord(), 19);
  EXPECT_EQ(b->asWord(), 2748);
  EXPECT_EQ(c->asWord(), 19);
  EXPECT_EQ(d->asWord(), 2748);
}

TEST(IntBuiltinsTest, CompareSmallIntEq) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCStr(R"(
a = 1
b = 2
a_eq_b = a == b
a_eq_a = a == a
b_eq_b = b == b
)");

  Module main(&scope, findModule(&runtime, "__main__"));
  Object a_eq_b(&scope, moduleAt(&runtime, main, "a_eq_b"));
  EXPECT_EQ(*a_eq_b, Bool::falseObj());
  Object a_eq_a(&scope, moduleAt(&runtime, main, "a_eq_a"));
  EXPECT_EQ(*a_eq_a, Bool::trueObj());
  Object b_eq_b(&scope, moduleAt(&runtime, main, "b_eq_b"));
  EXPECT_EQ(*b_eq_b, Bool::trueObj());
}

TEST(IntBuiltinsTest, CompareSmallIntGe) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCStr(R"(
a = 1
b = 2
a_ge_a = a >= a
a_ge_b = a >= b
b_ge_a = b >= a
b_ge_b = b >= b
)");

  Module main(&scope, findModule(&runtime, "__main__"));
  Object a_ge_a(&scope, moduleAt(&runtime, main, "a_ge_a"));
  EXPECT_EQ(*a_ge_a, Bool::trueObj());
  Object a_ge_b(&scope, moduleAt(&runtime, main, "a_ge_b"));
  EXPECT_EQ(*a_ge_b, Bool::falseObj());
  Object b_ge_a(&scope, moduleAt(&runtime, main, "b_ge_a"));
  EXPECT_EQ(*b_ge_a, Bool::trueObj());
  Object b_ge_b(&scope, moduleAt(&runtime, main, "b_ge_b"));
  EXPECT_EQ(*b_ge_b, Bool::trueObj());
}

TEST(IntBuiltinsTest, CompareSmallIntGt) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCStr(R"(
a = 1
b = 2
a_gt_a = a > a
a_gt_b = a > b
b_gt_a = b > a
b_gt_b = b > b
)");

  Module main(&scope, findModule(&runtime, "__main__"));
  Object a_gt_a(&scope, moduleAt(&runtime, main, "a_gt_a"));
  EXPECT_EQ(*a_gt_a, Bool::falseObj());
  Object a_gt_b(&scope, moduleAt(&runtime, main, "a_gt_b"));
  EXPECT_EQ(*a_gt_b, Bool::falseObj());
  Object b_gt_a(&scope, moduleAt(&runtime, main, "b_gt_a"));
  EXPECT_EQ(*b_gt_a, Bool::trueObj());
  Object b_gt_b(&scope, moduleAt(&runtime, main, "b_gt_b"));
  EXPECT_EQ(*b_gt_b, Bool::falseObj());
}

TEST(IntBuiltinsTest, CompareSmallIntLe) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCStr(R"(
a = 1
b = 2
a_le_a = a <= a
a_le_b = a <= b
b_le_a = b <= a
b_le_b = b <= b
)");

  Module main(&scope, findModule(&runtime, "__main__"));
  Object a_le_a(&scope, moduleAt(&runtime, main, "a_le_a"));
  EXPECT_EQ(*a_le_a, Bool::trueObj());
  Object a_le_b(&scope, moduleAt(&runtime, main, "a_le_b"));
  EXPECT_EQ(*a_le_b, Bool::trueObj());
  Object b_le_a(&scope, moduleAt(&runtime, main, "b_le_a"));
  EXPECT_EQ(*b_le_a, Bool::falseObj());
  Object b_le_b(&scope, moduleAt(&runtime, main, "b_le_b"));
  EXPECT_EQ(*b_le_b, Bool::trueObj());
}

TEST(IntBuiltinsTest, CompareSmallIntLt) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCStr(R"(
a = 1
b = 2
a_lt_a = a < a
a_lt_b = a < b
b_lt_a = b < a
b_lt_b = b < b
)");

  Module main(&scope, findModule(&runtime, "__main__"));
  Object a_lt_a(&scope, moduleAt(&runtime, main, "a_lt_a"));
  EXPECT_EQ(*a_lt_a, Bool::falseObj());
  Object a_lt_b(&scope, moduleAt(&runtime, main, "a_lt_b"));
  EXPECT_EQ(*a_lt_b, Bool::trueObj());
  Object b_lt_a(&scope, moduleAt(&runtime, main, "b_lt_a"));
  EXPECT_EQ(*b_lt_a, Bool::falseObj());
  Object b_lt_b(&scope, moduleAt(&runtime, main, "b_lt_b"));
  EXPECT_EQ(*b_lt_b, Bool::falseObj());
}

TEST(IntBuiltinsTest, CompareSmallIntNe) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCStr(R"(
a = 1
b = 2
a_ne_b = a != b
a_ne_a = a != a
b_ne_b = b != b
)");

  Module main(&scope, findModule(&runtime, "__main__"));
  Object a_ne_b(&scope, moduleAt(&runtime, main, "a_ne_b"));
  EXPECT_EQ(*a_ne_b, Bool::trueObj());
  Object a_ne_a(&scope, moduleAt(&runtime, main, "a_ne_a"));
  EXPECT_EQ(*a_ne_a, Bool::falseObj());
  Object b_ne_b(&scope, moduleAt(&runtime, main, "b_ne_b"));
  EXPECT_EQ(*b_ne_b, Bool::falseObj());
}

TEST(IntBuiltinsTest, CompareOpSmallInt) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCStr(R"(
a = 1
b = 2
c = 1
a_lt_b = a < b
a_le_b = a <= b
a_eq_b = a == b
a_ge_b = a >= b
a_gt_b = a > b
a_is_c = a is c
a_is_not_c = a is not c
)");

  Module main(&scope, findModule(&runtime, "__main__"));
  Object a_lt_b(&scope, moduleAt(&runtime, main, "a_lt_b"));
  EXPECT_EQ(*a_lt_b, Bool::trueObj());
  Object a_le_b(&scope, moduleAt(&runtime, main, "a_le_b"));
  EXPECT_EQ(*a_le_b, Bool::trueObj());
  Object a_eq_b(&scope, moduleAt(&runtime, main, "a_eq_b"));
  EXPECT_EQ(*a_eq_b, Bool::falseObj());
  Object a_ge_b(&scope, moduleAt(&runtime, main, "a_ge_b"));
  EXPECT_EQ(*a_ge_b, Bool::falseObj());
  Object a_gt_b(&scope, moduleAt(&runtime, main, "a_gt_b"));
  EXPECT_EQ(*a_gt_b, Bool::falseObj());
  Object a_is_c(&scope, moduleAt(&runtime, main, "a_is_c"));
  EXPECT_EQ(*a_is_c, Bool::trueObj());
  Object a_is_not_c(&scope, moduleAt(&runtime, main, "a_is_not_c"));
  EXPECT_EQ(*a_is_not_c, Bool::falseObj());
}

TEST(IntBuiltinsTest, UnaryInvertSmallInt) {
  Runtime runtime;
  HandleScope scope;

  const char* src = R"(
pos = 123
invert_pos = ~pos
neg = -456
invert_neg = ~neg
)";

  runtime.runFromCStr(src);

  Module main(&scope, findModule(&runtime, "__main__"));

  Object invert_pos(&scope, moduleAt(&runtime, main, "invert_pos"));
  ASSERT_TRUE(invert_pos->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*invert_pos)->value(), -124);

  Object invert_neg(&scope, moduleAt(&runtime, main, "invert_neg"));
  ASSERT_TRUE(invert_neg->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*invert_neg)->value(), 455);
}

TEST(IntBuiltinsTest, UnaryPositiveSmallInt) {
  Runtime runtime;
  HandleScope scope;

  const char* src = R"(
pos = 123
plus_pos = +pos
neg = -123
plus_neg = +neg
)";

  runtime.runFromCStr(src);

  Module main(&scope, findModule(&runtime, "__main__"));

  Object plus_pos(&scope, moduleAt(&runtime, main, "plus_pos"));
  ASSERT_TRUE(plus_pos->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*plus_pos)->value(), 123);

  Object plus_neg(&scope, moduleAt(&runtime, main, "plus_neg"));
  ASSERT_TRUE(plus_neg->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*plus_neg)->value(), -123);
}

TEST(IntBuiltinsTest, UnaryNegateSmallInt) {
  Runtime runtime;
  HandleScope scope;

  const char* src = R"(
pos = 123
minus_pos = -pos
neg = -123
minus_neg = -neg
)";

  runtime.runFromCStr(src);

  Module main(&scope, findModule(&runtime, "__main__"));

  Object minus_pos(&scope, moduleAt(&runtime, main, "minus_pos"));
  ASSERT_TRUE(minus_pos->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*minus_pos)->value(), -123);

  Object minus_neg(&scope, moduleAt(&runtime, main, "minus_neg"));
  ASSERT_TRUE(minus_neg->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*minus_neg)->value(), 123);
}

TEST(IntBuiltinsTest, TruthyIntPos) {
  const char* src = R"(
if 1:
  print("foo")
else:
  print("bar")
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "foo\n");
}

TEST(IntBuiltinsTest, TruthyIntNeg) {
  const char* src = R"(
if 0:
  print("foo")
else:
  print("bar")
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "bar\n");
}

TEST(IntBuiltinsTest, BinaryOps) {
  Runtime runtime;
  HandleScope scope;

  const char* src = R"(
a = 2
b = 3
c = 6
d = 7
print('a & b ==', a & b)
print('a ^ b ==', a ^ b)
print('a + b ==', a + b)

print('c // b ==', c // b)
print('d // b ==', d // b)

print('d % a ==', d % a)
print('d % b ==', d % b)

print('d * b ==', d * b)
print('c * b ==', c * b)

print('c - b ==', c - b)
print('b - c ==', b - c)

print('d * 0 ==', d * 0)
print('0 * d ==', 0 * d)
)";

  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, R"(a & b == 2
a ^ b == 1
a + b == 5
c // b == 2
d // b == 2
d % a == 1
d % b == 1
d * b == 21
c * b == 18
c - b == 3
b - c == -3
d * 0 == 0
0 * d == 0
)");
}

TEST(IntBuiltinsTest, BinaryMulOverflowCheck) {
  Runtime runtime;

  // Overflows in the multiplication itself.
  EXPECT_DEBUG_ONLY_DEATH(runtime.runFromCStr(R"(
a = 268435456
a = a * a * a
)"),
                          "small integer overflow");
}

TEST(IntBuiltinsTest, BinaryAddOverflowCheck) {
  Runtime runtime;
  HandleScope scope;

  Thread* thread = Thread::currentThread();
  Frame* frame = thread->openAndLinkFrame(0, 2, 0);
  frame->setLocal(0, SmallInt::fromWord(RawSmallInt::kMaxValue));
  frame->setLocal(1, SmallInt::fromWord(RawSmallInt::kMaxValue));
  Object result(&scope, SmallIntBuiltins::dunderAdd(thread, frame, 2));
  ASSERT_TRUE(result->isLargeInt());
  EXPECT_EQ(RawLargeInt::cast(*result)->asWord(), RawSmallInt::kMaxValue * 2);
}

TEST(IntBuiltinsTest, InplaceAdd) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCStr(R"(
a = 1
a += 0
b = a
a += 2
)");
  Module main(&scope, findModule(&runtime, "__main__"));
  Object a(&scope, moduleAt(&runtime, main, "a"));
  Object b(&scope, moduleAt(&runtime, main, "b"));
  ASSERT_TRUE(a->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*a)->value(), 3);
  ASSERT_TRUE(b->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*b)->value(), 1);
}

TEST(IntBuiltinsTest, InplaceMultiply) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCStr(R"(
a = 5
a *= 1
b = a
a *= 2
)");
  Module main(&scope, findModule(&runtime, "__main__"));
  Object a(&scope, moduleAt(&runtime, main, "a"));
  Object b(&scope, moduleAt(&runtime, main, "b"));
  ASSERT_TRUE(a->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*a)->value(), 10);
  ASSERT_TRUE(b->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*b)->value(), 5);
}

TEST(IntBuiltinsTest, InplaceFloorDiv) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCStr(R"(
a = 5
a //= 1
b = a
a //= 2
)");
  Module main(&scope, findModule(&runtime, "__main__"));
  Object a(&scope, moduleAt(&runtime, main, "a"));
  Object b(&scope, moduleAt(&runtime, main, "b"));
  ASSERT_TRUE(a->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*a)->value(), 2);
  ASSERT_TRUE(b->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*b)->value(), 5);
}

TEST(IntBuiltinsTest, InplaceModulo) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCStr(R"(
a = 10
a %= 7
b = a
a %= 2
)");
  Module main(&scope, findModule(&runtime, "__main__"));
  Object a(&scope, moduleAt(&runtime, main, "a"));
  Object b(&scope, moduleAt(&runtime, main, "b"));
  ASSERT_TRUE(a->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*a)->value(), 1);
  ASSERT_TRUE(b->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*b)->value(), 3);
}

TEST(IntBuiltinsTest, InplaceSub) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCStr(R"(
a = 10
a -= 0
b = a
a -= 7
)");
  Module main(&scope, findModule(&runtime, "__main__"));
  Object a(&scope, moduleAt(&runtime, main, "a"));
  Object b(&scope, moduleAt(&runtime, main, "b"));
  ASSERT_TRUE(a->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*a)->value(), 3);
  ASSERT_TRUE(b->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*b)->value(), 10);
}

TEST(IntBuiltinsTest, InplaceXor) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCStr(R"(
a = 0xFE
a ^= 0
b = a
a ^= 0x03
)");
  Module main(&scope, findModule(&runtime, "__main__"));
  Object a(&scope, moduleAt(&runtime, main, "a"));
  Object b(&scope, moduleAt(&runtime, main, "b"));
  ASSERT_TRUE(a->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*a)->value(), 0xFD);
  ASSERT_TRUE(b->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*b)->value(), 0xFE);
}

TEST(IntBuiltinsDeathTest, DunderOr) {
  Runtime runtime;
  HandleScope scope;
  runtime.runFromCStr(R"(
a = 0b010101
b = 0b111000
c = a | b
)");
  Object c(&scope, moduleAt(&runtime, "__main__", "c"));
  ASSERT_TRUE(c->isSmallInt());
  EXPECT_EQ(SmallInt::cast(*c)->value(), 0b111101);
}

TEST(IntBuiltinsDeathTest, DunderOrWithNonIntReturnsNotImplemented) {
  Runtime runtime;
  HandleScope scope;
  runtime.runFromCStr("a = int.__or__(10, '')");
  Object a(&scope, moduleAt(&runtime, "__main__", "a"));
  EXPECT_TRUE(a->isNotImplemented());
}

TEST(IntBuiltinsDeathTest, DunderOrWithInvalidArgumentThrowsException) {
  Runtime runtime;
  EXPECT_DEATH(runtime.runFromCStr("a = 10 | ''"), "Cannot do binary op");
  EXPECT_DEATH(runtime.runFromCStr("a = int.__or__('', 3)"),
               "descriptor '__or__' requires a 'int' object");
}

TEST(IntBuiltinsDeathTest, DunderLshift) {
  Runtime runtime;
  HandleScope scope;
  runtime.runFromCStr(R"(
a = 0b1101
b = a << 3
)");
  Object b(&scope, moduleAt(&runtime, "__main__", "b"));
  ASSERT_TRUE(b->isSmallInt());
  EXPECT_EQ(SmallInt::cast(*b)->value(), 0b1101000);
}

TEST(IntBuiltinsDeathTest, DunderLshiftWithNonIntReturnsNotImplemented) {
  Runtime runtime;
  HandleScope scope;
  runtime.runFromCStr("a = int.__lshift__(10, '')");
  Object a(&scope, moduleAt(&runtime, "__main__", "a"));
  EXPECT_TRUE(a->isNotImplemented());
}

TEST(IntBuiltinsDeathTest, DunderLshiftWithInvalidArgumentThrowsException) {
  Runtime runtime;
  EXPECT_DEATH(runtime.runFromCStr("a = 10 << ''"), "Cannot do binary op");
  EXPECT_DEATH(runtime.runFromCStr("a = int.__lshift__('', 3)"),
               "'__lshift__' requires a 'int' object");
  EXPECT_DEATH(runtime.runFromCStr("a = 10 << -3"), "negative shift count");
  EXPECT_DEATH(runtime.runFromCStr("a = 10 << (1 << 100)"),
               "shift count too large");
}

TEST(IntBuiltinsTest, BinaryAddSmallInt) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCStr(R"(
a = 2
b = 1
c = a + b
)");

  Module main(&scope, findModule(&runtime, "__main__"));
  Object c(&scope, moduleAt(&runtime, main, "c"));
  ASSERT_TRUE(c->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*c)->value(), 3);
}

TEST(IntBuiltinsTest, BinaryAddSmallIntOverflow) {
  Runtime runtime;
  HandleScope scope;

  Thread* thread = Thread::currentThread();
  Frame* frame = thread->openAndLinkFrame(0, 2, 0);

  frame->setLocal(0, SmallInt::fromWord(RawSmallInt::kMaxValue - 1));
  frame->setLocal(1, SmallInt::fromWord(2));
  Object c(&scope, SmallIntBuiltins::dunderAdd(thread, frame, 2));

  ASSERT_TRUE(c->isLargeInt());
  EXPECT_EQ(RawLargeInt::cast(*c)->asWord(), RawSmallInt::kMaxValue + 1);
}

TEST(IntBuiltinsTest, BitLength) {
  Runtime runtime;
  HandleScope scope;
  Thread* thread = Thread::currentThread();
  Frame* frame = thread->openAndLinkFrame(0, 1, 0);

  // (0).bit_length() == 0
  frame->setLocal(0, SmallInt::fromWord(0));
  Object bit_length(&scope, IntBuiltins::bitLength(thread, frame, 1));
  ASSERT_TRUE(bit_length->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*bit_length)->value(), 0);

  // (1).bit_length() == 1
  frame->setLocal(0, SmallInt::fromWord(1));
  Object bit_length1(&scope, IntBuiltins::bitLength(thread, frame, 1));
  ASSERT_TRUE(bit_length1->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*bit_length1)->value(), 1);

  // (-1).bit_length() == 1
  frame->setLocal(0, SmallInt::fromWord(1));
  Object bit_length2(&scope, IntBuiltins::bitLength(thread, frame, 1));
  ASSERT_TRUE(bit_length2->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*bit_length2)->value(), 1);

  // (SmallInt::kMaxValue).bit_length() == 62
  frame->setLocal(0, SmallInt::fromWord(RawSmallInt::kMaxValue));
  Object bit_length3(&scope, IntBuiltins::bitLength(thread, frame, 1));
  ASSERT_TRUE(bit_length3->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*bit_length3)->value(), 62);

  // (SmallInt::kMinValue).bit_length() == 63
  frame->setLocal(0, SmallInt::fromWord(RawSmallInt::kMinValue));
  Object bit_length4(&scope, IntBuiltins::bitLength(thread, frame, 1));
  ASSERT_TRUE(bit_length4->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*bit_length4)->value(), 63);

  // (kMaxInt64).bit_length() == 63
  Object large_int1(&scope, runtime.newInt(kMaxInt64));
  frame->setLocal(0, *large_int1);
  Object bit_length5(&scope, IntBuiltins::bitLength(thread, frame, 1));
  ASSERT_TRUE(bit_length5->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*bit_length5)->value(), 63);

  // (kMinInt64).bit_length() == 64
  Object large_int2(&scope, runtime.newInt(kMinInt64));
  frame->setLocal(0, *large_int2);
  Object bit_length6(&scope, IntBuiltins::bitLength(thread, frame, 1));
  ASSERT_TRUE(bit_length6->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*bit_length6)->value(), 64);

  word digits[] = {0, kMaxInt32};
  Object large_int3(&scope, runtime.newIntWithDigits(digits));
  frame->setLocal(0, *large_int3);
  Object bit_length7(&scope, IntBuiltins::bitLength(thread, frame, 1));
  ASSERT_TRUE(bit_length7->isSmallInt());
  // 31 bits for kMaxInt32 + 64 bits
  EXPECT_EQ(RawSmallInt::cast(*bit_length7)->value(), 95);

  // (kMinInt64 * 4).bit_length() == 66
  word digits2[] = {0, -2};
  Object large_int4(&scope, runtime.newIntWithDigits(digits2));
  frame->setLocal(0, *large_int4);
  Object bit_length8(&scope, IntBuiltins::bitLength(thread, frame, 1));
  ASSERT_TRUE(bit_length8->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*bit_length8)->value(), 66);

  // (kMinInt64 * 4 + 3).bit_length() == 65
  word digits3[] = {3, -2};
  Object large_int5(&scope, runtime.newIntWithDigits(digits3));
  frame->setLocal(0, *large_int5);
  Object bit_length9(&scope, IntBuiltins::bitLength(thread, frame, 1));
  ASSERT_TRUE(bit_length9->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*bit_length9)->value(), 65);
}

TEST(IntBuiltinsTest, CompareLargeIntEq) {
  Runtime runtime;
  HandleScope scope;

  Thread* thread = Thread::currentThread();
  Frame* frame = thread->openAndLinkFrame(0, 2, 0);

  Object a(&scope, runtime.newInt(RawSmallInt::kMaxValue + 1));
  Object b(&scope, runtime.newInt(RawSmallInt::kMinValue - 1));
  ASSERT_TRUE(a->isLargeInt());
  ASSERT_TRUE(b->isLargeInt());

  frame->setLocal(0, *a);
  frame->setLocal(1, *b);
  Object cmp_1(&scope, IntBuiltins::dunderEq(thread, frame, 2));
  ASSERT_TRUE(cmp_1->isBool());
  EXPECT_EQ(*cmp_1, Bool::falseObj());

  frame->setLocal(0, *a);
  frame->setLocal(1, SmallInt::fromWord(0));
  Object cmp_2(&scope, IntBuiltins::dunderEq(thread, frame, 2));
  ASSERT_TRUE(cmp_2->isBool());
  EXPECT_EQ(*cmp_2, Bool::falseObj());

  frame->setLocal(0, *a);
  frame->setLocal(1, *a);
  Object cmp_3(&scope, IntBuiltins::dunderEq(thread, frame, 2));
  ASSERT_TRUE(cmp_3->isBool());
  EXPECT_EQ(*cmp_3, Bool::trueObj());

  frame->setLocal(0, *b);
  frame->setLocal(1, *a);
  Object cmp_4(&scope, IntBuiltins::dunderEq(thread, frame, 2));
  ASSERT_TRUE(cmp_4->isBool());
  EXPECT_EQ(*cmp_4, Bool::falseObj());

  frame->setLocal(0, *b);
  frame->setLocal(1, SmallInt::fromWord(0));
  Object cmp_5(&scope, IntBuiltins::dunderEq(thread, frame, 2));
  ASSERT_TRUE(cmp_5->isBool());
  EXPECT_EQ(*cmp_5, Bool::falseObj());

  frame->setLocal(0, *b);
  frame->setLocal(1, *b);
  Object cmp_6(&scope, IntBuiltins::dunderEq(thread, frame, 2));
  ASSERT_TRUE(cmp_6->isBool());
  EXPECT_EQ(*cmp_6, Bool::trueObj());
}

TEST(IntBuiltinsTest, CompareLargeIntNe) {
  Runtime runtime;
  HandleScope scope;

  Thread* thread = Thread::currentThread();
  Frame* frame = thread->openAndLinkFrame(0, 2, 0);

  Object a(&scope, runtime.newInt(RawSmallInt::kMaxValue + 1));
  Object b(&scope, runtime.newInt(RawSmallInt::kMinValue - 1));
  ASSERT_TRUE(a->isLargeInt());
  ASSERT_TRUE(b->isLargeInt());

  frame->setLocal(0, *a);
  frame->setLocal(1, *b);
  Object cmp_1(&scope, IntBuiltins::dunderNe(thread, frame, 2));
  ASSERT_TRUE(cmp_1->isBool());
  EXPECT_EQ(*cmp_1, Bool::trueObj());

  frame->setLocal(0, *a);
  frame->setLocal(1, SmallInt::fromWord(0));
  Object cmp_2(&scope, IntBuiltins::dunderNe(thread, frame, 2));
  ASSERT_TRUE(cmp_2->isBool());
  EXPECT_EQ(*cmp_2, Bool::trueObj());

  frame->setLocal(0, *a);
  frame->setLocal(1, *a);
  Object cmp_3(&scope, IntBuiltins::dunderNe(thread, frame, 2));
  ASSERT_TRUE(cmp_3->isBool());
  EXPECT_EQ(*cmp_3, Bool::falseObj());

  frame->setLocal(0, *b);
  frame->setLocal(1, *a);
  Object cmp_4(&scope, IntBuiltins::dunderNe(thread, frame, 2));
  ASSERT_TRUE(cmp_4->isBool());
  EXPECT_EQ(*cmp_4, Bool::trueObj());

  frame->setLocal(0, *b);
  frame->setLocal(1, SmallInt::fromWord(0));
  Object cmp_5(&scope, IntBuiltins::dunderNe(thread, frame, 2));
  ASSERT_TRUE(cmp_5->isBool());
  EXPECT_EQ(*cmp_5, Bool::trueObj());

  frame->setLocal(0, *b);
  frame->setLocal(1, *b);
  Object cmp_6(&scope, IntBuiltins::dunderNe(thread, frame, 2));
  ASSERT_TRUE(cmp_6->isBool());
  EXPECT_EQ(*cmp_6, Bool::falseObj());
}

TEST(LargeIntBuiltinsTest, UnaryPositive) {
  Runtime runtime;
  HandleScope scope;
  Thread* thread = Thread::currentThread();
  Frame* frame = thread->openAndLinkFrame(0, 1, 0);

  Object smallint_max(&scope, runtime.newInt(RawSmallInt::kMaxValue));
  frame->setLocal(0, *smallint_max);
  Object a(&scope, IntBuiltins::dunderPos(thread, frame, 1));
  ASSERT_TRUE(a->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*a)->value(),
            static_cast<word>(RawSmallInt::kMaxValue));

  Object smallint_max1(&scope, runtime.newInt(RawSmallInt::kMaxValue + 1));
  frame->setLocal(0, *smallint_max1);
  Object b(&scope, IntBuiltins::dunderPos(thread, frame, 1));
  ASSERT_TRUE(b->isLargeInt());
  EXPECT_EQ(RawLargeInt::cast(*b)->asWord(), RawSmallInt::kMaxValue + 1);

  Object smallint_min(&scope, runtime.newInt(RawSmallInt::kMinValue));
  frame->setLocal(0, *smallint_min);
  Object c(&scope, IntBuiltins::dunderPos(thread, frame, 1));
  ASSERT_TRUE(c->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*c)->value(),
            static_cast<word>(RawSmallInt::kMinValue));

  Object smallint_min1(&scope, runtime.newInt(RawSmallInt::kMinValue - 1));
  frame->setLocal(0, *smallint_min1);
  Object d(&scope, IntBuiltins::dunderPos(thread, frame, 1));
  ASSERT_TRUE(d->isLargeInt());
  EXPECT_EQ(RawLargeInt::cast(*d)->asWord(), RawSmallInt::kMinValue - 1);
}

TEST(LargeIntBuiltinsTest, UnaryNegateTest) {
  Runtime runtime;
  HandleScope scope;
  Thread* thread = Thread::currentThread();
  Frame* frame = thread->openAndLinkFrame(0, 1, 0);

  Object smallint_max(&scope, runtime.newInt(RawSmallInt::kMaxValue));
  frame->setLocal(0, *smallint_max);
  Object a(&scope, IntBuiltins::dunderNeg(thread, frame, 1));
  ASSERT_TRUE(a->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*a)->value(), -RawSmallInt::kMaxValue);

  Object smallint_max1(&scope, runtime.newInt(RawSmallInt::kMaxValue + 1));
  frame->setLocal(0, *smallint_max1);
  Object b(&scope, IntBuiltins::dunderNeg(thread, frame, 1));
  ASSERT_TRUE(b->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*b)->value(), RawSmallInt::kMinValue);

  Object smallint_min(&scope, runtime.newInt(RawSmallInt::kMinValue));
  frame->setLocal(0, *smallint_min);
  Object c(&scope, IntBuiltins::dunderNeg(thread, frame, 1));
  ASSERT_TRUE(c->isLargeInt());
  EXPECT_EQ(RawLargeInt::cast(*c)->asWord(), -RawSmallInt::kMinValue);

  Object smallint_min1(&scope, runtime.newInt(RawSmallInt::kMinValue - 1));
  frame->setLocal(0, *smallint_min1);
  Object d(&scope, IntBuiltins::dunderNeg(thread, frame, 1));
  ASSERT_TRUE(d->isLargeInt());
  EXPECT_EQ(RawLargeInt::cast(*d)->asWord(), -(RawSmallInt::kMinValue - 1));

  Int min_word(&scope, runtime.newInt(kMinWord));
  frame->setLocal(0, *min_word);
  Object e(&scope, IntBuiltins::dunderNeg(thread, frame, 1));
  ASSERT_TRUE(e->isLargeInt());
  LargeInt large_e(&scope, *e);
  EXPECT_TRUE(large_e->isPositive());
  Int max_word(&scope, runtime.newInt(kMaxWord));
  EXPECT_EQ(RawInt::cast(*large_e)->compare(*max_word), 1);
  EXPECT_EQ(large_e->numDigits(), 2);
  EXPECT_EQ(large_e->digitAt(0), 1ULL << (kBitsPerWord - 1));
  EXPECT_EQ(large_e->digitAt(1), 0);
}

TEST(LargeIntBuiltinsTest, TruthyLargeInt) {
  const char* src = R"(
a = 4611686018427387903 + 1
if a:
  print("true")
else:
  print("false")
)";

  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "true\n");
}

TEST(IntBuiltinsTest, CompareLargeIntGe) {
  Runtime runtime;
  HandleScope scope;

  Thread* thread = Thread::currentThread();
  Frame* frame = thread->openAndLinkFrame(0, 2, 0);

  Object a(&scope, runtime.newInt(RawSmallInt::kMaxValue + 1));
  Object b(&scope, runtime.newInt(RawSmallInt::kMinValue - 1));
  ASSERT_TRUE(a->isLargeInt());
  ASSERT_TRUE(b->isLargeInt());

  frame->setLocal(0, *a);
  frame->setLocal(1, *b);
  Object cmp_1(&scope, IntBuiltins::dunderGe(thread, frame, 2));
  ASSERT_TRUE(cmp_1->isBool());
  EXPECT_EQ(*cmp_1, Bool::trueObj());

  frame->setLocal(0, *a);
  frame->setLocal(1, SmallInt::fromWord(0));
  Object cmp_2(&scope, IntBuiltins::dunderGe(thread, frame, 2));
  ASSERT_TRUE(cmp_2->isBool());
  EXPECT_EQ(*cmp_2, Bool::trueObj());

  frame->setLocal(0, *a);
  frame->setLocal(1, *a);
  Object cmp_3(&scope, IntBuiltins::dunderGe(thread, frame, 2));
  ASSERT_TRUE(cmp_3->isBool());
  EXPECT_EQ(*cmp_3, Bool::trueObj());

  frame->setLocal(0, *b);
  frame->setLocal(1, *a);
  Object cmp_4(&scope, IntBuiltins::dunderGe(thread, frame, 2));
  ASSERT_TRUE(cmp_4->isBool());
  EXPECT_EQ(*cmp_4, Bool::falseObj());

  frame->setLocal(0, *b);
  frame->setLocal(1, SmallInt::fromWord(0));
  Object cmp_5(&scope, IntBuiltins::dunderGe(thread, frame, 2));
  ASSERT_TRUE(cmp_5->isBool());
  EXPECT_EQ(*cmp_5, Bool::falseObj());

  frame->setLocal(0, *b);
  frame->setLocal(1, *b);
  Object cmp_6(&scope, IntBuiltins::dunderGe(thread, frame, 2));
  ASSERT_TRUE(cmp_6->isBool());
  EXPECT_EQ(*cmp_6, Bool::trueObj());
}

TEST(IntBuiltinsTest, CompareLargeIntLe) {
  Runtime runtime;
  HandleScope scope;

  Thread* thread = Thread::currentThread();
  Frame* frame = thread->openAndLinkFrame(0, 2, 0);

  Object a(&scope, runtime.newInt(RawSmallInt::kMaxValue + 1));
  Object b(&scope, runtime.newInt(RawSmallInt::kMinValue - 1));
  ASSERT_TRUE(a->isLargeInt());
  ASSERT_TRUE(b->isLargeInt());

  frame->setLocal(0, *a);
  frame->setLocal(1, *b);
  Object cmp_1(&scope, IntBuiltins::dunderLe(thread, frame, 2));
  ASSERT_TRUE(cmp_1->isBool());
  EXPECT_EQ(*cmp_1, Bool::falseObj());

  frame->setLocal(0, *a);
  frame->setLocal(1, SmallInt::fromWord(0));
  Object cmp_2(&scope, IntBuiltins::dunderLe(thread, frame, 2));
  ASSERT_TRUE(cmp_2->isBool());
  EXPECT_EQ(*cmp_2, Bool::falseObj());

  frame->setLocal(0, *a);
  frame->setLocal(1, *a);
  Object cmp_3(&scope, IntBuiltins::dunderLe(thread, frame, 2));
  ASSERT_TRUE(cmp_3->isBool());
  EXPECT_EQ(*cmp_3, Bool::trueObj());

  frame->setLocal(0, *b);
  frame->setLocal(1, *a);
  Object cmp_4(&scope, IntBuiltins::dunderLe(thread, frame, 2));
  ASSERT_TRUE(cmp_4->isBool());
  EXPECT_EQ(*cmp_4, Bool::trueObj());

  frame->setLocal(0, *b);
  frame->setLocal(1, SmallInt::fromWord(0));
  Object cmp_5(&scope, IntBuiltins::dunderLe(thread, frame, 2));
  ASSERT_TRUE(cmp_5->isBool());
  EXPECT_EQ(*cmp_5, Bool::trueObj());

  frame->setLocal(0, *b);
  frame->setLocal(1, *b);
  Object cmp_6(&scope, IntBuiltins::dunderLe(thread, frame, 2));
  ASSERT_TRUE(cmp_6->isBool());
  EXPECT_EQ(*cmp_6, Bool::trueObj());
}

TEST(IntBuiltinsTest, CompareLargeIntGt) {
  Runtime runtime;
  HandleScope scope;

  Thread* thread = Thread::currentThread();
  Frame* frame = thread->openAndLinkFrame(0, 2, 0);

  Object a(&scope, runtime.newInt(RawSmallInt::kMaxValue + 1));
  Object b(&scope, runtime.newInt(RawSmallInt::kMinValue - 1));
  ASSERT_TRUE(a->isLargeInt());
  ASSERT_TRUE(b->isLargeInt());

  frame->setLocal(0, *a);
  frame->setLocal(1, *b);
  Object cmp_1(&scope, IntBuiltins::dunderGt(thread, frame, 2));
  ASSERT_TRUE(cmp_1->isBool());
  EXPECT_EQ(*cmp_1, Bool::trueObj());

  frame->setLocal(0, *a);
  frame->setLocal(1, SmallInt::fromWord(0));
  Object cmp_2(&scope, IntBuiltins::dunderGt(thread, frame, 2));
  ASSERT_TRUE(cmp_2->isBool());
  EXPECT_EQ(*cmp_2, Bool::trueObj());

  frame->setLocal(0, *a);
  frame->setLocal(1, *a);
  Object cmp_3(&scope, IntBuiltins::dunderGt(thread, frame, 2));
  ASSERT_TRUE(cmp_3->isBool());
  EXPECT_EQ(*cmp_3, Bool::falseObj());

  frame->setLocal(0, *b);
  frame->setLocal(1, *a);
  Object cmp_4(&scope, IntBuiltins::dunderGt(thread, frame, 2));
  ASSERT_TRUE(cmp_4->isBool());
  EXPECT_EQ(*cmp_4, Bool::falseObj());

  frame->setLocal(0, *b);
  frame->setLocal(1, SmallInt::fromWord(0));
  Object cmp_5(&scope, IntBuiltins::dunderGt(thread, frame, 2));
  ASSERT_TRUE(cmp_5->isBool());
  EXPECT_EQ(*cmp_5, Bool::falseObj());

  frame->setLocal(0, *b);
  frame->setLocal(1, *b);
  Object cmp_6(&scope, IntBuiltins::dunderGt(thread, frame, 2));
  ASSERT_TRUE(cmp_6->isBool());
  EXPECT_EQ(*cmp_6, Bool::falseObj());
}

TEST(IntBuiltinsTest, CompareLargeIntLt) {
  Runtime runtime;
  HandleScope scope;

  Thread* thread = Thread::currentThread();
  Frame* frame = thread->openAndLinkFrame(0, 2, 0);

  Object a(&scope, runtime.newInt(RawSmallInt::kMaxValue + 1));
  Object b(&scope, runtime.newInt(RawSmallInt::kMinValue - 1));
  ASSERT_TRUE(a->isLargeInt());
  ASSERT_TRUE(b->isLargeInt());

  frame->setLocal(0, *a);
  frame->setLocal(1, *b);
  Object cmp_1(&scope, IntBuiltins::dunderLt(thread, frame, 2));
  ASSERT_TRUE(cmp_1->isBool());
  EXPECT_EQ(*cmp_1, Bool::falseObj());

  frame->setLocal(0, *a);
  frame->setLocal(1, SmallInt::fromWord(0));
  Object cmp_2(&scope, IntBuiltins::dunderLt(thread, frame, 2));
  ASSERT_TRUE(cmp_2->isBool());
  EXPECT_EQ(*cmp_2, Bool::falseObj());

  frame->setLocal(0, *a);
  frame->setLocal(1, *a);
  Object cmp_3(&scope, IntBuiltins::dunderLt(thread, frame, 2));
  ASSERT_TRUE(cmp_3->isBool());
  EXPECT_EQ(*cmp_3, Bool::falseObj());

  frame->setLocal(0, *b);
  frame->setLocal(1, *a);
  Object cmp_4(&scope, IntBuiltins::dunderLt(thread, frame, 2));
  ASSERT_TRUE(cmp_4->isBool());
  EXPECT_EQ(*cmp_4, Bool::trueObj());

  frame->setLocal(0, *b);
  frame->setLocal(1, SmallInt::fromWord(0));
  Object cmp_5(&scope, IntBuiltins::dunderLt(thread, frame, 2));
  ASSERT_TRUE(cmp_5->isBool());
  EXPECT_EQ(*cmp_5, Bool::trueObj());

  frame->setLocal(0, *b);
  frame->setLocal(1, *b);
  Object cmp_6(&scope, IntBuiltins::dunderLt(thread, frame, 2));
  ASSERT_TRUE(cmp_6->isBool());
  EXPECT_EQ(*cmp_6, Bool::falseObj());
}

TEST(IntBuiltinsTest, StringToIntDPos) {
  Runtime runtime;
  HandleScope scope;
  Thread* thread = Thread::currentThread();

  Object str_d0(&scope, runtime.newStrFromCStr("0"));
  SmallInt int_d0(&scope, IntBuiltins::intFromString(thread, *str_d0, 10));
  EXPECT_EQ(int_d0->value(), 0);

  Object str_d123(&scope, runtime.newStrFromCStr("123"));
  SmallInt int_d123(&scope, IntBuiltins::intFromString(thread, *str_d123, 10));
  EXPECT_EQ(int_d123->value(), 123);

  Object str_d987n(&scope, runtime.newStrFromCStr("-987"));
  SmallInt int_d987n(&scope,
                     IntBuiltins::intFromString(thread, *str_d987n, 10));
  EXPECT_EQ(int_d987n->value(), -987);
}

TEST(IntBuiltinsTest, StringToIntDNeg) {
  Runtime runtime;
  HandleScope scope;
  Thread* thread = Thread::currentThread();

  Object str1(&scope, runtime.newStrFromCStr(""));
  Object res1(&scope, IntBuiltins::intFromString(thread, *str1, 10));
  EXPECT_TRUE(res1->isError());

  Object str2(&scope, runtime.newStrFromCStr("12ab"));
  Object res2(&scope, IntBuiltins::intFromString(thread, *str2, 10));
  EXPECT_TRUE(res2->isError());
}

TEST(IntBuiltinsTest, DunderIndexReturnsSameValue) {
  Runtime runtime;
  HandleScope scope;
  Thread* thread = Thread::currentThread();

  runtime.runFromCStr(R"(
a = (7).__index__()
b = int.__index__(7)
)");

  Module main(&scope, findModule(&runtime, "__main__"));
  Object a(&scope, moduleAt(&runtime, main, "a"));
  Object b(&scope, moduleAt(&runtime, main, "b"));
  ASSERT_TRUE(a->isSmallInt());
  ASSERT_TRUE(b->isSmallInt());
  EXPECT_EQ(7, RawSmallInt::cast(*a)->value());
  EXPECT_EQ(7, RawSmallInt::cast(*b)->value());
}

TEST(IntBuiltinsTest, DunderIntReturnsSameValue) {
  Runtime runtime;
  HandleScope scope;
  Thread* thread = Thread::currentThread();

  runtime.runFromCStr(R"(
a = (7).__int__()
b = int.__int__(7)
)");
  Module main(&scope, findModule(&runtime, "__main__"));
  Object a(&scope, moduleAt(&runtime, main, "a"));
  Object b(&scope, moduleAt(&runtime, main, "b"));
  ASSERT_TRUE(a->isSmallInt());
  ASSERT_TRUE(b->isSmallInt());
  EXPECT_EQ(7, RawSmallInt::cast(*a)->value());
  EXPECT_EQ(7, RawSmallInt::cast(*b)->value());

  Frame* frame = thread->openAndLinkFrame(0, 1, 0);
  frame->setLocal(0, runtime.newStrFromCStr("python"));
  Object res(&scope, IntBuiltins::dunderInt(thread, frame, 1));
  EXPECT_TRUE(res->isError());
}

TEST(IntBuiltinsTest, DunderIntOnBool) {
  Runtime runtime;
  HandleScope scope;
  Thread* thread = Thread::currentThread();
  Frame* frame = thread->openAndLinkFrame(0, 1, 0);

  frame->setLocal(0, Bool::trueObj());
  Object a(&scope, IntBuiltins::dunderInt(thread, frame, 1));
  ASSERT_TRUE(a->isSmallInt());
  EXPECT_EQ(1, RawSmallInt::cast(*a)->value());

  frame->setLocal(0, Bool::falseObj());
  Object b(&scope, IntBuiltins::dunderInt(thread, frame, 1));
  ASSERT_TRUE(b->isSmallInt());
  EXPECT_EQ(0, RawSmallInt::cast(*b)->value());
}

TEST(IntBuiltinsTest, DunderBoolOnBool) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  Frame* frame = thread->openAndLinkFrame(0, 1, 0);

  frame->setLocal(0, Bool::trueObj());
  RawObject result = IntBuiltins::dunderBool(thread, frame, 1);
  EXPECT_EQ(result, Bool::trueObj());

  frame->setLocal(0, Bool::falseObj());
  RawObject result1 = IntBuiltins::dunderBool(thread, frame, 1);
  EXPECT_EQ(result1, Bool::falseObj());
}

TEST(IntBuiltinsTest, BitLengthOnBool) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  Frame* frame = thread->openAndLinkFrame(0, 1, 0);

  frame->setLocal(0, Bool::trueObj());
  RawObject result = IntBuiltins::dunderBool(thread, frame, 1);
  EXPECT_EQ(result, Bool::trueObj());

  frame->setLocal(0, Bool::falseObj());
  RawObject result1 = IntBuiltins::dunderBool(thread, frame, 1);
  EXPECT_EQ(result1, Bool::falseObj());
}

TEST(IntBuiltinsTest, DunderEqOnBool) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  Frame* frame = thread->openAndLinkFrame(0, 2, 0);

  frame->setLocal(0, Bool::trueObj());
  frame->setLocal(1, Bool::trueObj());
  RawObject result = IntBuiltins::dunderEq(thread, frame, 2);
  EXPECT_EQ(result, Bool::trueObj());

  frame->setLocal(1, Bool::falseObj());
  RawObject result1 = IntBuiltins::dunderEq(thread, frame, 2);
  EXPECT_EQ(result1, Bool::falseObj());

  frame->setLocal(1, SmallInt::fromWord(0));
  RawObject result2 = IntBuiltins::dunderEq(thread, frame, 2);
  EXPECT_EQ(result2, Bool::falseObj());

  frame->setLocal(1, SmallInt::fromWord(1));
  RawObject result3 = IntBuiltins::dunderEq(thread, frame, 2);
  EXPECT_EQ(result3, Bool::trueObj());
}

TEST(IntBuiltinsTest, DunderNeOnBool) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  Frame* frame = thread->openAndLinkFrame(0, 2, 0);

  frame->setLocal(0, Bool::trueObj());
  frame->setLocal(1, Bool::trueObj());
  RawObject result = IntBuiltins::dunderNe(thread, frame, 2);
  EXPECT_EQ(result, Bool::falseObj());

  frame->setLocal(1, Bool::falseObj());
  RawObject result1 = IntBuiltins::dunderNe(thread, frame, 2);
  EXPECT_EQ(result1, Bool::trueObj());

  frame->setLocal(1, SmallInt::fromWord(0));
  RawObject result2 = IntBuiltins::dunderNe(thread, frame, 2);
  EXPECT_EQ(result2, Bool::trueObj());

  frame->setLocal(1, SmallInt::fromWord(1));
  RawObject result3 = IntBuiltins::dunderNe(thread, frame, 2);
  EXPECT_EQ(result3, Bool::falseObj());
}

TEST(IntBuiltinsTest, DunderNegOnBool) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  Frame* frame = thread->openAndLinkFrame(0, 1, 0);

  frame->setLocal(0, Bool::trueObj());
  RawObject result = IntBuiltins::dunderNeg(thread, frame, 1);
  EXPECT_EQ(result, SmallInt::fromWord(-1));

  frame->setLocal(0, Bool::falseObj());
  RawObject result1 = IntBuiltins::dunderNeg(thread, frame, 1);
  EXPECT_EQ(result1, SmallInt::fromWord(0));
}

TEST(IntBuiltinsTest, DunderPosOnBool) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  Frame* frame = thread->openAndLinkFrame(0, 1, 0);

  frame->setLocal(0, Bool::trueObj());
  RawObject result = IntBuiltins::dunderPos(thread, frame, 1);
  EXPECT_EQ(result, SmallInt::fromWord(1));

  frame->setLocal(0, Bool::falseObj());
  RawObject result1 = IntBuiltins::dunderPos(thread, frame, 1);
  EXPECT_EQ(result1, SmallInt::fromWord(0));
}

TEST(IntBuiltinsTest, DunderLtOnBool) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  Frame* frame = thread->openAndLinkFrame(0, 2, 0);

  frame->setLocal(0, Bool::trueObj());
  frame->setLocal(1, Bool::falseObj());
  RawObject result = IntBuiltins::dunderLt(thread, frame, 2);
  EXPECT_EQ(result, Bool::falseObj());

  frame->setLocal(0, Bool::falseObj());
  frame->setLocal(1, Bool::trueObj());
  RawObject result1 = IntBuiltins::dunderLt(thread, frame, 2);
  EXPECT_EQ(result1, Bool::trueObj());

  frame->setLocal(0, Bool::falseObj());
  frame->setLocal(1, SmallInt::fromWord(1));
  RawObject result2 = IntBuiltins::dunderLt(thread, frame, 2);
  EXPECT_EQ(result2, Bool::trueObj());

  frame->setLocal(0, Bool::falseObj());
  frame->setLocal(1, SmallInt::fromWord(-1));
  RawObject result3 = IntBuiltins::dunderLt(thread, frame, 2);
  EXPECT_EQ(result3, Bool::falseObj());
}

TEST(IntBuiltinsTest, DunderGeOnBool) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  Frame* frame = thread->openAndLinkFrame(0, 2, 0);

  frame->setLocal(0, Bool::trueObj());
  frame->setLocal(1, Bool::falseObj());
  RawObject result = IntBuiltins::dunderGe(thread, frame, 2);
  EXPECT_EQ(result, Bool::trueObj());

  frame->setLocal(0, Bool::falseObj());
  frame->setLocal(1, Bool::trueObj());
  RawObject result1 = IntBuiltins::dunderGe(thread, frame, 2);
  EXPECT_EQ(result1, Bool::falseObj());

  frame->setLocal(0, Bool::falseObj());
  frame->setLocal(1, SmallInt::fromWord(1));
  RawObject result2 = IntBuiltins::dunderGe(thread, frame, 2);
  EXPECT_EQ(result2, Bool::falseObj());

  frame->setLocal(0, Bool::falseObj());
  frame->setLocal(1, SmallInt::fromWord(-1));
  RawObject result3 = IntBuiltins::dunderGe(thread, frame, 2);
  EXPECT_EQ(result3, Bool::trueObj());
}

TEST(IntBuiltinsTest, DunderGtOnBool) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  Frame* frame = thread->openAndLinkFrame(0, 2, 0);

  frame->setLocal(0, Bool::trueObj());
  frame->setLocal(1, Bool::falseObj());
  RawObject result = IntBuiltins::dunderGt(thread, frame, 2);
  EXPECT_EQ(result, Bool::trueObj());

  frame->setLocal(0, Bool::falseObj());
  frame->setLocal(1, Bool::trueObj());
  RawObject result1 = IntBuiltins::dunderGt(thread, frame, 2);
  EXPECT_EQ(result1, Bool::falseObj());

  frame->setLocal(0, Bool::falseObj());
  frame->setLocal(1, SmallInt::fromWord(1));
  RawObject result2 = IntBuiltins::dunderGt(thread, frame, 2);
  EXPECT_EQ(result2, Bool::falseObj());

  frame->setLocal(0, Bool::falseObj());
  frame->setLocal(1, SmallInt::fromWord(-1));
  RawObject result3 = IntBuiltins::dunderGt(thread, frame, 2);
  EXPECT_EQ(result3, Bool::trueObj());
}

TEST(IntBuiltinsTest, DunderLeOnBool) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  Frame* frame = thread->openAndLinkFrame(0, 2, 0);

  frame->setLocal(0, Bool::trueObj());
  frame->setLocal(1, Bool::falseObj());
  RawObject result = IntBuiltins::dunderLe(thread, frame, 2);
  EXPECT_EQ(result, Bool::falseObj());

  frame->setLocal(0, Bool::falseObj());
  frame->setLocal(1, Bool::trueObj());
  RawObject result1 = IntBuiltins::dunderLe(thread, frame, 2);
  EXPECT_EQ(result1, Bool::trueObj());

  frame->setLocal(0, Bool::falseObj());
  frame->setLocal(1, SmallInt::fromWord(1));
  RawObject result2 = IntBuiltins::dunderLe(thread, frame, 2);
  EXPECT_EQ(result2, Bool::trueObj());

  frame->setLocal(0, Bool::falseObj());
  frame->setLocal(1, SmallInt::fromWord(-1));
  RawObject result3 = IntBuiltins::dunderLe(thread, frame, 2);
  EXPECT_EQ(result3, Bool::falseObj());
}

TEST(IntBuiltinsTest, SmallIntDunderRepr) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Frame* frame = thread->openAndLinkFrame(1, 0, 0);

  frame->setLocal(0, SmallInt::fromWord(RawSmallInt::kMinValue));
  Str str(&scope, SmallIntBuiltins::dunderRepr(thread, frame, 1));
  EXPECT_PYSTRING_EQ(*str, "-4611686018427387904");

  frame->setLocal(0, SmallInt::fromWord(RawSmallInt::kMaxValue));
  str = SmallIntBuiltins::dunderRepr(thread, frame, 1);
  EXPECT_PYSTRING_EQ(*str, "4611686018427387903");

  frame->setLocal(0, SmallInt::fromWord(0));
  str = SmallIntBuiltins::dunderRepr(thread, frame, 1);
  EXPECT_PYSTRING_EQ(*str, "0");

  frame->setLocal(0, SmallInt::fromWord(0xdeadbeef));
  str = SmallIntBuiltins::dunderRepr(thread, frame, 1);
  EXPECT_PYSTRING_EQ(*str, "3735928559");
}

TEST(BoolBuiltinsTest, NewFromNonZeroIntegerReturnsTrue) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  Frame* frame = thread->openAndLinkFrame(0, 2, 0);
  frame->setLocal(0, runtime.typeAt(LayoutId::kBool));
  frame->setLocal(1, SmallInt::fromWord(2));
  RawObject result = BoolBuiltins::dunderNew(thread, frame, 2);
  EXPECT_TRUE(RawBool::cast(result)->value());
}

TEST(BoolBuiltinsTest, NewFromZerorReturnsFalse) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  Frame* frame = thread->openAndLinkFrame(0, 2, 0);
  frame->setLocal(0, runtime.typeAt(LayoutId::kBool));
  frame->setLocal(1, SmallInt::fromWord(0));
  RawObject result = BoolBuiltins::dunderNew(thread, frame, 2);
  EXPECT_FALSE(RawBool::cast(result)->value());
}

TEST(BoolBuiltinsTest, NewFromTrueReturnsTrue) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  Frame* frame = thread->openAndLinkFrame(0, 2, 0);
  frame->setLocal(0, runtime.typeAt(LayoutId::kBool));
  frame->setLocal(1, Bool::trueObj());
  RawObject result = BoolBuiltins::dunderNew(thread, frame, 2);
  EXPECT_TRUE(RawBool::cast(result)->value());
  thread->popFrame();
}

TEST(BoolBuiltinsTest, NewFromFalseReturnsTrue) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  Frame* frame = thread->openAndLinkFrame(0, 2, 0);
  frame->setLocal(0, runtime.typeAt(LayoutId::kBool));
  frame->setLocal(1, Bool::falseObj());
  RawObject result = BoolBuiltins::dunderNew(thread, frame, 2);
  EXPECT_FALSE(RawBool::cast(result)->value());
  thread->popFrame();
}

TEST(BoolBuiltinsTest, NewFromNoneIsFalse) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();

  Frame* frame = thread->openAndLinkFrame(0, 2, 0);
  frame->setLocal(0, runtime.typeAt(LayoutId::kBool));
  frame->setLocal(1, NoneType::object());
  RawObject result = BoolBuiltins::dunderNew(thread, frame, 2);
  EXPECT_FALSE(RawBool::cast(result)->value());
  thread->popFrame();
}

TEST(BoolBuiltinsTest, NewFromUserDefinedType) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  runtime.runFromCStr(R"(
class Foo:
  def __bool__(self):
    return True

class Bar:
  def __bool__(self):
    return False

foo = Foo()
bar = Bar()
)");
  HandleScope scope;
  Module main(&scope, findModule(&runtime, "__main__"));
  Object foo(&scope, moduleAt(&runtime, main, "foo"));
  Object bar(&scope, moduleAt(&runtime, main, "bar"));

  {
    Frame* frame = thread->openAndLinkFrame(0, 2, 0);
    frame->setLocal(0, runtime.typeAt(LayoutId::kBool));
    frame->setLocal(1, *foo);
    RawObject result = BoolBuiltins::dunderNew(thread, frame, 2);
    EXPECT_TRUE(RawBool::cast(result)->value());
    thread->popFrame();
  }
  {
    Frame* frame = thread->openAndLinkFrame(0, 2, 0);
    frame->setLocal(0, runtime.typeAt(LayoutId::kBool));
    frame->setLocal(1, *bar);
    RawObject result = BoolBuiltins::dunderNew(thread, frame, 2);
    EXPECT_FALSE(RawBool::cast(result)->value());
    thread->popFrame();
  }
}

TEST(SmallIntBuiltinsDeathTest, DunderModZeroDivision) {
  Runtime runtime;
  EXPECT_DEATH(runtime.runFromCStr(R"(
a = 10
b = 0.0
a % b
)"),
               "float modulo");

  EXPECT_DEATH(runtime.runFromCStr(R"(
a = 10
b = False
a % b
)"),
               "integer division or modulo by zero");

  EXPECT_DEATH(runtime.runFromCStr(R"(
a = 10
b = 0
a % b
)"),
               "integer division or modulo by zero");
}

TEST(SmallIntBuiltinsDeathTest, DunderFloorDivZeroDivision) {
  Runtime runtime;
  EXPECT_DEATH(runtime.runFromCStr(R"(
a = 10
b = 0.0
a // b
)"),
               "float divmod()");

  EXPECT_DEATH(runtime.runFromCStr(R"(
a = 10
b = False
a // b
)"),
               "integer division or modulo by zero");

  EXPECT_DEATH(runtime.runFromCStr(R"(
a = 10
b = 0
a // b
)"),
               "integer division or modulo by zero");
}

TEST(SmallIntBuiltinsDeathTest, DunderTrueDivZeroDivision) {
  Runtime runtime;
  EXPECT_DEATH(runtime.runFromCStr(R"(
a = 10
b = 0.0
a / b
)"),
               "float division by zero");

  EXPECT_DEATH(runtime.runFromCStr(R"(
a = 10
b = False
a / b
)"),
               "division by zero");

  EXPECT_DEATH(runtime.runFromCStr(R"(
a = 10
b = 0
a / b
)"),
               "division by zero");
}

TEST(SmallIntBuiltinsTest, DunderModWithFloat) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Frame* frame = thread->openAndLinkFrame(0, 2, 0);

  // Test positive smallint mod positive float
  Float float1(&scope, runtime.newFloat(1.5));
  frame->setLocal(0, SmallInt::fromWord(100));
  frame->setLocal(1, *float1);
  Float result(&scope, SmallIntBuiltins::dunderMod(thread, frame, 2));
  EXPECT_NEAR(result->value(), 1.0, DBL_EPSILON);

  // Test positive smallint mod negative float
  Float float2(&scope, runtime.newFloat(-1.5));
  frame->setLocal(0, SmallInt::fromWord(100));
  frame->setLocal(1, *float2);
  Float result1(&scope, SmallIntBuiltins::dunderMod(thread, frame, 2));
  EXPECT_NEAR(result1->value(), -0.5, DBL_EPSILON);

  // Test positive smallint mod infinity
  Float float_inf(&scope, runtime.newFloat(INFINITY));
  frame->setLocal(0, SmallInt::fromWord(100));
  frame->setLocal(1, *float_inf);
  Float result2(&scope, SmallIntBuiltins::dunderMod(thread, frame, 2));
  ASSERT_TRUE(result2->isFloat());
  EXPECT_NEAR(result2->value(), 100.0, DBL_EPSILON);

  // Test positive smallint mod negative infinity
  Float neg_float_inf(&scope, runtime.newFloat(-INFINITY));
  frame->setLocal(0, SmallInt::fromWord(100));
  frame->setLocal(1, *neg_float_inf);
  Float result3(&scope, SmallIntBuiltins::dunderMod(thread, frame, 2));
  EXPECT_EQ(result3->value(), -INFINITY);

  // Test negative smallint mod infinity
  frame->setLocal(0, SmallInt::fromWord(-100));
  frame->setLocal(1, *float_inf);
  Float result4(&scope, SmallIntBuiltins::dunderMod(thread, frame, 2));
  EXPECT_EQ(result4->value(), INFINITY);

  // Test negative smallint mod negative infinity
  frame->setLocal(0, SmallInt::fromWord(-100));
  frame->setLocal(1, *neg_float_inf);
  Float result5(&scope, SmallIntBuiltins::dunderMod(thread, frame, 2));
  EXPECT_NEAR(result5->value(), -100.0, DBL_EPSILON);

  // Test negative smallint mod nan
  Float nan(&scope, runtime.newFloat(NAN));
  frame->setLocal(0, SmallInt::fromWord(-100));
  frame->setLocal(1, *nan);
  Float result6(&scope, SmallIntBuiltins::dunderMod(thread, frame, 2));
  EXPECT_TRUE(std::isnan(result6->value()));

  thread->popFrame();
}

TEST(SmallIntBuiltinsTest, DunderFloorDivWithFloat) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Frame* frame = thread->openAndLinkFrame(0, 2, 0);

  // Test dividing a positive smallint by a positive float
  Float float1(&scope, runtime.newFloat(1.5));
  frame->setLocal(0, SmallInt::fromWord(100));
  frame->setLocal(1, *float1);
  Float result(&scope, SmallIntBuiltins::dunderFloorDiv(thread, frame, 2));
  EXPECT_NEAR(result->value(), 66.0, DBL_EPSILON);

  // Test dividing a positive smallint by a negative float
  Float float2(&scope, runtime.newFloat(-1.5));
  frame->setLocal(0, SmallInt::fromWord(100));
  frame->setLocal(1, *float2);
  Float result1(&scope, SmallIntBuiltins::dunderFloorDiv(thread, frame, 2));
  EXPECT_NEAR(result1->value(), -67.0, DBL_EPSILON);

  // Test dividing a positive smallint by infinity
  Float float_inf(&scope, runtime.newFloat(INFINITY));
  frame->setLocal(0, SmallInt::fromWord(100));
  frame->setLocal(1, *float_inf);
  Float result2(&scope, SmallIntBuiltins::dunderFloorDiv(thread, frame, 2));
  EXPECT_NEAR(result2->value(), 0.0, DBL_EPSILON);

  // Test dividing a positive smallint by negative infinity
  Float neg_float_inf(&scope, runtime.newFloat(-INFINITY));
  frame->setLocal(0, SmallInt::fromWord(100));
  frame->setLocal(1, *neg_float_inf);
  Float result3(&scope, SmallIntBuiltins::dunderFloorDiv(thread, frame, 2));
  EXPECT_NEAR(result3->value(), 0.0, DBL_EPSILON);

  // Test dividing a negative smallint by infinity
  frame->setLocal(0, SmallInt::fromWord(-100));
  frame->setLocal(1, *float_inf);
  Float result4(&scope, SmallIntBuiltins::dunderFloorDiv(thread, frame, 2));
  EXPECT_NEAR(result4->value(), 0.0, DBL_EPSILON);

  // Test dividing a negative smallint by negative infinity
  frame->setLocal(0, SmallInt::fromWord(-100));
  frame->setLocal(1, *neg_float_inf);
  Float result5(&scope, SmallIntBuiltins::dunderFloorDiv(thread, frame, 2));
  EXPECT_NEAR(result5->value(), 0.0, DBL_EPSILON);

  // Test dividing negative smallint by nan
  Float nan(&scope, runtime.newFloat(NAN));
  frame->setLocal(0, SmallInt::fromWord(-100));
  frame->setLocal(1, *nan);
  Float result6(&scope, SmallIntBuiltins::dunderFloorDiv(thread, frame, 2));
  EXPECT_TRUE(std::isnan(result6->value()));

  thread->popFrame();
}

TEST(SmallIntBuiltinsTest, DunderTrueDivWithFloat) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Frame* frame = thread->openAndLinkFrame(0, 2, 0);

  // Test dividing a positive smallint by a positive float
  Float float1(&scope, runtime.newFloat(1.5));
  frame->setLocal(0, SmallInt::fromWord(100));
  frame->setLocal(1, *float1);
  Float result(&scope, SmallIntBuiltins::dunderTrueDiv(thread, frame, 2));
  EXPECT_NEAR(result->value(), 66.66666666666667, DBL_EPSILON);

  // Test dividing a positive smallint by a negative float
  Float float2(&scope, runtime.newFloat(-1.5));
  frame->setLocal(0, SmallInt::fromWord(100));
  frame->setLocal(1, *float2);
  Float result1(&scope, SmallIntBuiltins::dunderTrueDiv(thread, frame, 2));
  EXPECT_NEAR(result1->value(), -66.66666666666667, DBL_EPSILON);

  // Test dividing a positive smallint by infinity
  Float float_inf(&scope, runtime.newFloat(INFINITY));
  frame->setLocal(0, SmallInt::fromWord(100));
  frame->setLocal(1, *float_inf);
  Float result2(&scope, SmallIntBuiltins::dunderTrueDiv(thread, frame, 2));
  EXPECT_NEAR(result2->value(), 0.0, DBL_EPSILON);

  // Test dividing a positive smallint by negative infinity
  Float neg_float_inf(&scope, runtime.newFloat(-INFINITY));
  frame->setLocal(0, SmallInt::fromWord(100));
  frame->setLocal(1, *neg_float_inf);
  Float result3(&scope, SmallIntBuiltins::dunderTrueDiv(thread, frame, 2));
  EXPECT_NEAR(result3->value(), 0.0, DBL_EPSILON);

  // Test dividing a negative smallint by infinity
  frame->setLocal(0, SmallInt::fromWord(-100));
  frame->setLocal(1, *float_inf);
  Float result4(&scope, SmallIntBuiltins::dunderTrueDiv(thread, frame, 2));
  EXPECT_NEAR(result4->value(), 0.0, DBL_EPSILON);

  // Test dividing a negative smallint by negative infinity
  frame->setLocal(0, SmallInt::fromWord(-100));
  frame->setLocal(1, *neg_float_inf);
  Float result5(&scope, SmallIntBuiltins::dunderTrueDiv(thread, frame, 2));
  EXPECT_NEAR(result5->value(), 0.0, DBL_EPSILON);

  // Test dividing negative smallint by nan
  Float nan(&scope, runtime.newFloat(NAN));
  frame->setLocal(0, SmallInt::fromWord(-100));
  frame->setLocal(1, *nan);
  Float result6(&scope, SmallIntBuiltins::dunderTrueDiv(thread, frame, 2));
  EXPECT_TRUE(std::isnan(result6->value()));

  thread->popFrame();
}

TEST(SmallIntBuiltinsTest, DunderTrueDivWithSmallInt) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Frame* frame = thread->openAndLinkFrame(0, 2, 0);

  frame->setLocal(0, SmallInt::fromWord(6));
  frame->setLocal(1, SmallInt::fromWord(3));
  Float result(&scope, SmallIntBuiltins::dunderTrueDiv(thread, frame, 2));
  EXPECT_NEAR(result->value(), 2.0, DBL_EPSILON);

  frame->setLocal(0, SmallInt::fromWord(7));
  frame->setLocal(1, SmallInt::fromWord(3));
  Float result1(&scope, SmallIntBuiltins::dunderTrueDiv(thread, frame, 2));
  EXPECT_NEAR(result1->value(), 2.3333333333333335, DBL_EPSILON);

  thread->popFrame();
}

}  // namespace python

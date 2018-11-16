#include "gtest/gtest.h"

#include "utils.h"

namespace python {

TEST(UtilsTest, RotateLeft) {
  EXPECT_EQ(Utils::rotateLeft(1ULL, 0), 0x0000000000000001);
  EXPECT_EQ(Utils::rotateLeft(1ULL, 1), 0x0000000000000002);
  EXPECT_EQ(Utils::rotateLeft(1ULL, 2), 0x0000000000000004);
  EXPECT_EQ(Utils::rotateLeft(1ULL, 3), 0x0000000000000008);
  EXPECT_EQ(Utils::rotateLeft(1ULL, 4), 0x0000000000000010);
  EXPECT_EQ(Utils::rotateLeft(1ULL, 5), 0x0000000000000020);
  EXPECT_EQ(Utils::rotateLeft(1ULL, 6), 0x0000000000000040);
  EXPECT_EQ(Utils::rotateLeft(1ULL, 7), 0x0000000000000080);
  EXPECT_EQ(Utils::rotateLeft(1ULL, 8), 0x0000000000000100);
  EXPECT_EQ(Utils::rotateLeft(1ULL, 9), 0x0000000000000200);
  EXPECT_EQ(Utils::rotateLeft(1ULL, 10), 0x0000000000000400);
  EXPECT_EQ(Utils::rotateLeft(1ULL, 11), 0x0000000000000800);
  EXPECT_EQ(Utils::rotateLeft(1ULL, 12), 0x0000000000001000);
  EXPECT_EQ(Utils::rotateLeft(1ULL, 13), 0x0000000000002000);
  EXPECT_EQ(Utils::rotateLeft(1ULL, 14), 0x0000000000004000);
  EXPECT_EQ(Utils::rotateLeft(1ULL, 15), 0x0000000000008000);
  EXPECT_EQ(Utils::rotateLeft(1ULL, 16), 0x0000000000010000);
  EXPECT_EQ(Utils::rotateLeft(1ULL, 17), 0x0000000000020000);
  EXPECT_EQ(Utils::rotateLeft(1ULL, 18), 0x0000000000040000);
  EXPECT_EQ(Utils::rotateLeft(1ULL, 19), 0x0000000000080000);
  EXPECT_EQ(Utils::rotateLeft(1ULL, 20), 0x0000000000100000);
  EXPECT_EQ(Utils::rotateLeft(1ULL, 21), 0x0000000000200000);
  EXPECT_EQ(Utils::rotateLeft(1ULL, 22), 0x0000000000400000);
  EXPECT_EQ(Utils::rotateLeft(1ULL, 23), 0x0000000000800000);
  EXPECT_EQ(Utils::rotateLeft(1ULL, 24), 0x0000000001000000);
  EXPECT_EQ(Utils::rotateLeft(1ULL, 25), 0x0000000002000000);
  EXPECT_EQ(Utils::rotateLeft(1ULL, 26), 0x0000000004000000);
  EXPECT_EQ(Utils::rotateLeft(1ULL, 27), 0x0000000008000000);
  EXPECT_EQ(Utils::rotateLeft(1ULL, 28), 0x0000000010000000);
  EXPECT_EQ(Utils::rotateLeft(1ULL, 29), 0x0000000020000000);
  EXPECT_EQ(Utils::rotateLeft(1ULL, 30), 0x0000000040000000);
  EXPECT_EQ(Utils::rotateLeft(1ULL, 31), 0x0000000080000000);
  EXPECT_EQ(Utils::rotateLeft(1ULL, 32), 0x0000000100000000);
  EXPECT_EQ(Utils::rotateLeft(1ULL, 33), 0x0000000200000000);
  EXPECT_EQ(Utils::rotateLeft(1ULL, 34), 0x0000000400000000);
  EXPECT_EQ(Utils::rotateLeft(1ULL, 35), 0x0000000800000000);
  EXPECT_EQ(Utils::rotateLeft(1ULL, 36), 0x0000001000000000);
  EXPECT_EQ(Utils::rotateLeft(1ULL, 37), 0x0000002000000000);
  EXPECT_EQ(Utils::rotateLeft(1ULL, 38), 0x0000004000000000);
  EXPECT_EQ(Utils::rotateLeft(1ULL, 39), 0x0000008000000000);
  EXPECT_EQ(Utils::rotateLeft(1ULL, 40), 0x0000010000000000);
  EXPECT_EQ(Utils::rotateLeft(1ULL, 41), 0x0000020000000000);
  EXPECT_EQ(Utils::rotateLeft(1ULL, 42), 0x0000040000000000);
  EXPECT_EQ(Utils::rotateLeft(1ULL, 43), 0x0000080000000000);
  EXPECT_EQ(Utils::rotateLeft(1ULL, 44), 0x0000100000000000);
  EXPECT_EQ(Utils::rotateLeft(1ULL, 45), 0x0000200000000000);
  EXPECT_EQ(Utils::rotateLeft(1ULL, 46), 0x0000400000000000);
  EXPECT_EQ(Utils::rotateLeft(1ULL, 47), 0x0000800000000000);
  EXPECT_EQ(Utils::rotateLeft(1ULL, 48), 0x0001000000000000);
  EXPECT_EQ(Utils::rotateLeft(1ULL, 49), 0x0002000000000000);
  EXPECT_EQ(Utils::rotateLeft(1ULL, 50), 0x0004000000000000);
  EXPECT_EQ(Utils::rotateLeft(1ULL, 51), 0x0008000000000000);
  EXPECT_EQ(Utils::rotateLeft(1ULL, 52), 0x0010000000000000);
  EXPECT_EQ(Utils::rotateLeft(1ULL, 53), 0x0020000000000000);
  EXPECT_EQ(Utils::rotateLeft(1ULL, 54), 0x0040000000000000);
  EXPECT_EQ(Utils::rotateLeft(1ULL, 55), 0x0080000000000000);
  EXPECT_EQ(Utils::rotateLeft(1ULL, 56), 0x0100000000000000);
  EXPECT_EQ(Utils::rotateLeft(1ULL, 57), 0x0200000000000000);
  EXPECT_EQ(Utils::rotateLeft(1ULL, 58), 0x0400000000000000);
  EXPECT_EQ(Utils::rotateLeft(1ULL, 59), 0x0800000000000000);
  EXPECT_EQ(Utils::rotateLeft(1ULL, 60), 0x1000000000000000);
  EXPECT_EQ(Utils::rotateLeft(1ULL, 61), 0x2000000000000000);
  EXPECT_EQ(Utils::rotateLeft(1ULL, 62), 0x4000000000000000);
  EXPECT_EQ(Utils::rotateLeft(1ULL, 63), 0x8000000000000000);
}

} // namespace python

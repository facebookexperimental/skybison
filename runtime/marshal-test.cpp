#include "gtest/gtest.h"

#include <cstdint>

#include "globals.h"
#include "marshal.h"
#include "runtime.h"

namespace python {

TEST(MarshalReaderTest, ReadString) {
  Runtime runtime;
  HandleScope scope;
  Marshal::Reader reader(&scope, &runtime, "hello, world");

  const byte* s1 = reader.readString(1);
  ASSERT_NE(s1, nullptr);
  EXPECT_EQ(*s1, 'h');

  const byte* s2 = reader.readString(2);
  ASSERT_NE(s2, nullptr);
  EXPECT_EQ(s2[0], 'e');
  EXPECT_EQ(s2[1], 'l');
}

TEST(MarshalReaderTest, ReadLong) {
  Runtime runtime;
  HandleScope scope;

  int32 a = Marshal::Reader(&scope, &runtime, "\x01\x00\x00\x00").readLong();
  EXPECT_EQ(a, 1);

  int32 b = Marshal::Reader(&scope, &runtime, "\x01\x02\x00\x00").readLong();
  ASSERT_EQ(b, 0x0201);

  int32 c = Marshal::Reader(&scope, &runtime, "\x01\x02\x03\x00").readLong();
  ASSERT_EQ(c, 0x030201);

  int32 d = Marshal::Reader(&scope, &runtime, "\x01\x02\x03\x04").readLong();
  ASSERT_EQ(d, 0x04030201);

  int32 e = Marshal::Reader(&scope, &runtime, "\x00\x00\x00\x80").readLong();
  ASSERT_EQ(e, -2147483648); // INT32_MIN
}

TEST(MarshalReaderTest, ReadTypeIntMin) {
  Runtime runtime;
  HandleScope scope;

  // marshal.dumps(INT32_MIN)
  Marshal::Reader reader(&scope, &runtime, "\xe9\x00\x00\x00\x80");
  Object* result = reader.readObject();
  ASSERT_TRUE(result->isSmallInteger());
  EXPECT_EQ(SmallInteger::cast(result)->value(), INT32_MIN);
  ASSERT_EQ(reader.numRefs(), 1);
  EXPECT_EQ(reader.getRef(0), result);

  // marshal.dumps(INT32_MIN)
  Marshal::Reader reader_norefs(&scope, &runtime, "\x69\x00\x00\x00\x80");
  result = reader_norefs.readObject();
  ASSERT_TRUE(result->isSmallInteger());
  EXPECT_EQ(SmallInteger::cast(result)->value(), INT32_MIN);
  EXPECT_EQ(reader_norefs.numRefs(), 0);
}

TEST(MarshalReaderTest, ReadTypeIntMax) {
  Runtime runtime;
  HandleScope scope;

  // marshal.dumps(INT32_MAX)
  Marshal::Reader reader(&scope, &runtime, "\xe9\xff\xff\xff\x7f");
  Object* result = reader.readObject();
  ASSERT_TRUE(result->isSmallInteger());
  EXPECT_EQ(SmallInteger::cast(result)->value(), INT32_MAX);
  ASSERT_EQ(reader.numRefs(), 1);
  EXPECT_EQ(reader.getRef(0), result);

  // marshal.dumps(INT32_MAX)
  Marshal::Reader reader_norefs(&scope, &runtime, "\x69\xff\xff\xff\x7f");
  result = reader_norefs.readObject();
  ASSERT_TRUE(result->isSmallInteger());
  EXPECT_EQ(SmallInteger::cast(result)->value(), INT32_MAX);
  EXPECT_EQ(reader_norefs.numRefs(), 0);
}

TEST(MarshalReaderDeathTest, ReadNegativeTypeLong) {
  Runtime runtime;
  HandleScope scope;

  // marshal.dumps(INT32_MIN - 1)
  const char buf[] = "\xec\xfd\xff\xff\xff\x01\x00\x00\x00\x02\x00";
  EXPECT_DEATH(
      Marshal::Reader(&scope, &runtime, buf).readObject(),
      "Cannot handle TYPE_LONG");
}

TEST(MarshalReaderDeathTest, ReadPositiveTypeLong) {
  Runtime runtime;
  HandleScope scope;

  // marshal.dumps(INT32_MAX + 1)
  const char buf[] = "\xec\x03\x00\x00\x00\x00\x00\x00\x00\x02\x00";
  EXPECT_DEATH(
      Marshal::Reader(&scope, &runtime, buf).readObject(),
      "Cannot handle TYPE_LONG");
}

TEST(MarshalReaderTest, ReadShort) {
  Runtime runtime;
  HandleScope scope;

  int16 a = Marshal::Reader(&scope, &runtime, "\x01\x00").readShort();
  EXPECT_EQ(a, 1);

  int16 b = Marshal::Reader(&scope, &runtime, "\x01\x02").readShort();
  ASSERT_EQ(b, 0x0201);

  int16 c = Marshal::Reader(&scope, &runtime, "\x00\x80").readShort();
  ASSERT_EQ(c, -32768); // INT16_MIN
}

TEST(MarshalReaderTest, ReadObjectNull) {
  Runtime runtime;
  HandleScope scope;
  Object* a = Marshal::Reader(&scope, &runtime, "0").readObject();
  ASSERT_EQ(a, nullptr);
}

TEST(MarshalReaderTest, ReadObjectCode) {
  Runtime runtime;
  HandleScope scope;
  const char* buffer =
      "\x33\x0D\x0D\x0A\x3B\x5B\xB8\x59\x05\x00\x00\x00\xE3\x00\x00\x00\x00\x00"
      "\x00\x00\x00\x00\x00\x00\x00\x01\x00\x00\x00\x40\x00\x00\x00\x73\x04\x00"
      "\x00\x00\x64\x00\x53\x00\x29\x01\x4E\xA9\x00\x72\x01\x00\x00\x00\x72\x01"
      "\x00\x00\x00\x72\x01\x00\x00\x00\xFA\x07\x70\x61\x73\x73\x2E\x70\x79\xDA"
      "\x08\x3C\x6D\x6F\x64\x75\x6C\x65\x3E\x01\x00\x00\x00\x73\x00\x00\x00\x00";
  Marshal::Reader reader(&scope, &runtime, buffer);

  int32 magic = reader.readLong();
  EXPECT_EQ(magic, 0x0A0D0D33);
  int32 mtime = reader.readLong();
  EXPECT_EQ(mtime, 0x59B85B3B);
  int32 size = reader.readLong();
  EXPECT_EQ(size, 0x05);

  Object* raw_object = reader.readObject();
  ASSERT_TRUE(raw_object->isCode());

  Code* code = Code::cast(raw_object);
  EXPECT_EQ(code->argcount(), 0);
  EXPECT_EQ(code->kwonlyargcount(), 0);
  EXPECT_EQ(code->nlocals(), 0);
  EXPECT_EQ(code->stacksize(), 1);
  EXPECT_EQ(code->cell2arg(), 0);
  EXPECT_EQ(code->flags(), 0x00000040);

  ASSERT_TRUE(code->code()->isByteArray());
  EXPECT_NE(ByteArray::cast(code->code())->length(), 0);

  ASSERT_TRUE(code->varnames()->isObjectArray());
  EXPECT_EQ(ObjectArray::cast(code->varnames())->length(), 0);

  ASSERT_TRUE(code->cellvars()->isObjectArray());
  EXPECT_EQ(ObjectArray::cast(code->cellvars())->length(), 0);

  ASSERT_TRUE(code->consts()->isObjectArray());
  ASSERT_EQ(ObjectArray::cast(code->consts())->length(), 1);
  EXPECT_EQ(ObjectArray::cast(code->consts())->at(0), None::object());

  ASSERT_TRUE(code->freevars()->isObjectArray());
  EXPECT_EQ(ObjectArray::cast(code->freevars())->length(), 0);

  ASSERT_TRUE(code->filename()->isString());
  EXPECT_TRUE(String::cast(code->filename())->equalsCString("pass.py"));

  ASSERT_TRUE(code->name()->isString());
  EXPECT_TRUE(String::cast(code->name())->equalsCString("<module>"));

  ASSERT_TRUE(code->names()->isObjectArray());
  EXPECT_EQ(ObjectArray::cast(code->names())->length(), 0);

  EXPECT_EQ(code->firstlineno(), 1);

  ASSERT_TRUE(code->lnotab()->isByteArray());
  EXPECT_EQ(ByteArray::cast(code->lnotab())->length(), 0);
}

} // namespace python

#include "gtest/gtest.h"

#include <iostream>
#include <memory>

#include "builtins.h"
#include "bytecode.h"
#include "frame.h"
#include "globals.h"
#include "interpreter.h"
#include "marshal.h"
#include "runtime.h"
#include "test-utils.h"
#include "thread.h"
#include "trampolines-inl.h"

namespace python {
using namespace testing;

TEST(ThreadTest, CheckMainThreadRuntime) {
  Runtime runtime;
  auto thread = Thread::currentThread();
  ASSERT_EQ(thread->runtime(), &runtime);
}

TEST(ThreadTest, RunEmptyFunction) {
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
  EXPECT_EQ(size, 5);

  auto code = reader.readObject();
  ASSERT_TRUE(code->isCode());
  EXPECT_EQ(Code::cast(code)->argcount(), 0);

  Thread thread(1 * KiB);
  Object* result = thread.run(code);
  ASSERT_EQ(result, None::object()); // returns None
}

TEST(ThreadTest, RunHelloWorld) {
  Runtime runtime;
  HandleScope scope;
  const char* buffer =
      "\x33\x0D\x0D\x0A\x1B\x69\xC1\x59\x16\x00\x00\x00\xE3\x00\x00\x00\x00\x00"
      "\x00\x00\x00\x00\x00\x00\x00\x02\x00\x00\x00\x40\x00\x00\x00\x73\x0C\x00"
      "\x00\x00\x65\x00\x64\x00\x83\x01\x01\x00\x64\x01\x53\x00\x29\x02\x7A\x0C"
      "\x68\x65\x6C\x6C\x6F\x2C\x20\x77\x6F\x72\x6C\x64\x4E\x29\x01\xDA\x05\x70"
      "\x72\x69\x6E\x74\xA9\x00\x72\x02\x00\x00\x00\x72\x02\x00\x00\x00\xFA\x0D"
      "\x68\x65\x6C\x6C\x6F\x77\x6F\x72\x6C\x64\x2E\x70\x79\xDA\x08\x3C\x6D\x6F"
      "\x64\x75\x6C\x65\x3E\x01\x00\x00\x00\x73\x00\x00\x00\x00";

  // Execute the code and make sure we get back the result we expect
  std::string result = runToString(&runtime, buffer);
  EXPECT_EQ(result, "hello, world\n");
}

TEST(ThreadTest, ModuleBodyCallsHelloWorldFunction) {
  Runtime runtime;
  const char* src = R"(
def hello():
  print('hello, world')
hello()
)";

  std::unique_ptr<char[]> buffer(Runtime::compile(src));

  // Execute the code and make sure we get back the result we expect
  std::string output = runToString(&runtime, buffer.get());
  EXPECT_EQ(output, "hello, world\n");
}

TEST(ThreadTest, DunderCallClass) {
  Runtime runtime;
  HandleScope scope;

  const char* src = R"(
class C: pass
c = C()
)";

  std::unique_ptr<char[]> buffer(Runtime::compile(src));
  runtime.run(buffer.get());

  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Class> type(&scope, moduleAt(&runtime, main, "C"));
  ASSERT_FALSE(type->isError());
  Handle<Object> instance(&scope, moduleAt(&runtime, main, "c"));
  ASSERT_FALSE(instance->isError());
  Handle<Object> instance_type(&scope, runtime.classOf(*instance));
  ASSERT_FALSE(instance_type->isError());

  EXPECT_EQ(*type, *instance_type);
}

TEST(ThreadTest, DunderCallClassWithInit) {
  Runtime runtime;
  HandleScope scope;

  const char* src = R"(
class C:
  def __init__(self):
    global g
    g = 2

g = 1
C()
)";

  std::unique_ptr<char[]> buffer(Runtime::compile(src));
  runtime.run(buffer.get());

  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Object> global(&scope, moduleAt(&runtime, main, "g"));
  ASSERT_FALSE(global->isError());
  ASSERT_TRUE(global->isSmallInteger());
  EXPECT_EQ(SmallInteger::cast(*global)->value(), 2);
}

TEST(ThreadTest, DunderCallClassWithInitAndArgs) {
  Runtime runtime;
  HandleScope scope;

  const char* src = R"(
class C:
  def __init__(self, x):
    global g
    g = x

g = 1
C(9)
)";

  std::unique_ptr<char[]> buffer(Runtime::compile(src));
  runtime.run(buffer.get());

  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Object> global(&scope, moduleAt(&runtime, main, "g"));
  ASSERT_FALSE(global->isError());
  ASSERT_TRUE(global->isSmallInteger());
  EXPECT_EQ(SmallInteger::cast(*global)->value(), 9);
}

TEST(ThreadTest, DunderCallInstance) {
  Runtime runtime;
  HandleScope scope;

  const char* src = R"(
class C:
  def __init__(self, x, y):
    self.value = x + y
  def __call__(self, y):
    return self.value * y
c = C(10, 2)
g = c(3)
)";

  std::unique_ptr<char[]> buffer(Runtime::compile(src));
  runtime.run(buffer.get());

  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Object> global(&scope, moduleAt(&runtime, main, "g"));
  ASSERT_FALSE(global->isError());
  ASSERT_TRUE(global->isSmallInteger());
  EXPECT_EQ(SmallInteger::cast(*global)->value(), 36);
}

TEST(ThreadTest, OverlappingFrames) {
  Runtime runtime;
  HandleScope scope;

  // Push a frame for a code object with space for 3 items on the value stack
  Handle<Code> callerCode(&scope, runtime.newCode());
  callerCode->setStacksize(3);
  auto thread = Thread::currentThread();
  auto callerFrame = thread->pushFrame(*callerCode);
  Object** sp = callerFrame->valueStackTop();
  // Push args on the stack in the sequence generated by CPython
  auto arg1 = SmallInteger::fromWord(1111);
  *--sp = arg1;
  auto arg2 = SmallInteger::fromWord(2222);
  *--sp = arg2;
  auto arg3 = SmallInteger::fromWord(3333);
  *--sp = arg3;
  callerFrame->setValueStackTop(sp);

  // Push a frame for a code object that expects 3 arguments and needs space
  // for 3 local variables
  Handle<Code> code(&scope, runtime.newCode());
  code->setArgcount(3);
  code->setNlocals(3);
  auto frame = thread->pushFrame(*code);

  // Make sure we can read the args from the frame
  Object* local = frame->getLocal(0);
  ASSERT_TRUE(local->isSmallInteger());
  EXPECT_EQ(SmallInteger::cast(local)->value(), arg1->value());

  local = frame->getLocal(1);
  ASSERT_TRUE(local->isSmallInteger());
  EXPECT_EQ(SmallInteger::cast(local)->value(), arg2->value());

  local = frame->getLocal(2);
  ASSERT_TRUE(local->isSmallInteger());
  EXPECT_EQ(SmallInteger::cast(local)->value(), arg3->value());
}

TEST(ThreadTest, EncodeTryBlock) {
  TryBlock block(100, 200, 300);
  EXPECT_EQ(block.kind(), 100);
  EXPECT_EQ(block.handler(), 200);
  EXPECT_EQ(block.level(), 300);

  TryBlock decoded(block.asSmallInteger());
  EXPECT_EQ(decoded.kind(), block.kind());
  EXPECT_EQ(decoded.handler(), block.handler());
  EXPECT_EQ(decoded.level(), block.level());
}

TEST(ThreadTest, PushPopFrame) {
  Runtime runtime;
  HandleScope scope;

  Handle<Code> code(&scope, runtime.newCode());
  code->setNlocals(2);
  code->setStacksize(3);

  auto thread = Thread::currentThread();
  byte* prevSp = thread->stackPtr();
  auto frame = thread->pushFrame(*code);

  // Verify frame invariants post-push
  EXPECT_EQ(frame->previousFrame(), thread->initialFrame());
  EXPECT_EQ(frame->code(), *code);
  EXPECT_EQ(frame->valueStackTop(), reinterpret_cast<Object**>(frame));
  EXPECT_EQ(frame->valueStackBase(), frame->valueStackTop());
  EXPECT_EQ(frame->numLocals(), 2);

  // Make sure we restore the thread's stack pointer back to its previous
  // location
  thread->popFrame();
  EXPECT_EQ(thread->stackPtr(), prevSp);
}

TEST(ThreadTest, PushFrameWithNoCellVars) {
  Runtime runtime;
  HandleScope scope;

  Handle<Code> code(&scope, runtime.newCode());
  code->setCellvars(None::object());
  code->setFreevars(runtime.newObjectArray(0));
  auto thread = Thread::currentThread();
  byte* prevSp = thread->stackPtr();
  thread->pushFrame(*code);

  EXPECT_EQ(thread->stackPtr(), prevSp - Frame::kSize);
}

TEST(ThreadTest, PushFrameWithNoFreeVars) {
  Runtime runtime;
  HandleScope scope;

  Handle<Code> code(&scope, runtime.newCode());
  code->setFreevars(None::object());
  code->setCellvars(runtime.newObjectArray(0));
  auto thread = Thread::currentThread();
  byte* prevSp = thread->stackPtr();
  thread->pushFrame(*code);

  EXPECT_EQ(thread->stackPtr(), prevSp - Frame::kSize);
}

TEST(ThreadTest, ZeroInitializeBlockStack) {
  Runtime runtime;
  auto thread = Thread::currentThread();
  Frame* frame1 = thread->openAndLinkFrame(0, 0, 10);
  Object** sp = frame1->valueStackTop();
  for (word i = 0; i < 10; i++) {
    *sp-- = SmallInteger::fromWord(1111);
  }
  Frame* frame2 = thread->openAndLinkFrame(0, 0, 10);
  // The block stack is a contiguous chunk of small integers.
  Object** bs = reinterpret_cast<Object**>(frame2->blockStack());
  for (word i = 0; i < BlockStack::kSize / kPointerSize; i++) {
    EXPECT_EQ(bs[i], SmallInteger::fromWord(0));
  }
}

TEST(ThreadTest, ManipulateValueStack) {
  Runtime runtime;
  HandleScope scope;
  auto thread = Thread::currentThread();
  auto frame = thread->openAndLinkFrame(0, 0, 3);

  // Push 3 items on the value stack
  Object** sp = frame->valueStackTop();
  *--sp = SmallInteger::fromWord(1111);
  *--sp = SmallInteger::fromWord(2222);
  *--sp = SmallInteger::fromWord(3333);
  frame->setValueStackTop(sp);
  ASSERT_EQ(frame->valueStackTop(), sp);

  // Verify the value stack is laid out as we expect
  word values[] = {3333, 2222, 1111};
  for (int i = 0; i < 3; i++) {
    Object* object = frame->peek(i);
    ASSERT_TRUE(object->isSmallInteger())
        << "Value at stack depth " << i << " is not an integer";
    EXPECT_EQ(SmallInteger::cast(object)->value(), values[i])
        << "Incorrect value at stack depth " << i;
  }

  // Pop 2 items off the stack and check the stack is still as we expect
  frame->setValueStackTop(sp + 2);
  Object* top = frame->peek(0);
  ASSERT_TRUE(top->isSmallInteger()) << "Stack top isn't an integer";
  EXPECT_EQ(SmallInteger::cast(top)->value(), 1111)
      << "Incorrect value for stack top";
}

TEST(ThreadTest, ManipulateBlockStack) {
  Runtime runtime;
  HandleScope scope;
  auto thread = Thread::currentThread();
  auto frame = thread->openAndLinkFrame(0, 0, 0);
  BlockStack* blockStack = frame->blockStack();

  TryBlock pushed1(Bytecode::SETUP_LOOP, 100, 10);
  blockStack->push(pushed1);

  TryBlock pushed2(Bytecode::SETUP_EXCEPT, 200, 20);
  blockStack->push(pushed2);

  TryBlock popped2 = blockStack->pop();
  EXPECT_EQ(popped2.kind(), pushed2.kind());
  EXPECT_EQ(popped2.handler(), pushed2.handler());
  EXPECT_EQ(popped2.level(), pushed2.level());

  TryBlock popped1 = blockStack->pop();
  EXPECT_EQ(popped1.kind(), pushed1.kind());
  EXPECT_EQ(popped1.handler(), pushed1.handler());
  EXPECT_EQ(popped1.level(), pushed1.level());
}

TEST(ThreadTest, CallFunction) {
  Runtime runtime;
  HandleScope scope;

  // Build the code object for the following function
  //
  //     def noop(a, b):
  //         return 2222
  //
  auto expectedResult = SmallInteger::fromWord(2222);
  Handle<Code> calleeCode(&scope, runtime.newCode());
  calleeCode->setArgcount(2);
  calleeCode->setStacksize(1);
  calleeCode->setConsts(runtime.newObjectArray(1));
  ObjectArray::cast(calleeCode->consts())->atPut(0, expectedResult);
  const byte callee_bc[] = {LOAD_CONST, 0, RETURN_VALUE, 0};
  calleeCode->setCode(runtime.newByteArrayWithAll(callee_bc));

  // Create the function object and bind it to the code object
  Handle<Function> callee(&scope, runtime.newFunction());
  callee->setCode(*calleeCode);
  callee->setEntry(interpreterTrampoline);

  // Build a code object to call the function defined above
  Handle<Code> callerCode(&scope, runtime.newCode());
  callerCode->setStacksize(3);
  Handle<ObjectArray> consts(&scope, runtime.newObjectArray(3));
  consts->atPut(0, *callee);
  consts->atPut(1, SmallInteger::fromWord(1111));
  consts->atPut(2, SmallInteger::fromWord(2222));
  callerCode->setConsts(*consts);
  const byte caller_bc[] = {LOAD_CONST,
                            0,
                            LOAD_CONST,
                            1,
                            LOAD_CONST,
                            2,
                            CALL_FUNCTION,
                            2,
                            RETURN_VALUE,
                            0};
  callerCode->setCode(runtime.newByteArrayWithAll(caller_bc));

  // Execute the caller and make sure we get back the expected result
  Object* result = Thread::currentThread()->run(*callerCode);
  ASSERT_TRUE(result->isSmallInteger());
  EXPECT_EQ(SmallInteger::cast(result)->value(), expectedResult->value());
}

static Object* firstArg(Thread*, Frame* callerFrame, word argc)
    __attribute__((aligned(16)));

Object* firstArg(Thread*, Frame* callerFrame, word argc) {
  if (argc == 0) {
    return None::object();
  }
  return *(callerFrame->valueStackTop() + argc - 1);
}

TEST(ThreadTest, CallBuiltinFunction) {
  Runtime runtime;
  HandleScope scope;

  // Create the builtin function
  Handle<Function> callee(&scope, runtime.newFunction());
  callee->setEntry(firstArg);

  // Set up a code object that calls the builtin with a single argument.
  Handle<Code> code(&scope, runtime.newCode());
  Handle<ObjectArray> consts(&scope, runtime.newObjectArray(2));
  consts->atPut(0, *callee);
  consts->atPut(1, SmallInteger::fromWord(1111));
  code->setConsts(*consts);
  const byte bytecode[] = {
      LOAD_CONST, 0, LOAD_CONST, 1, CALL_FUNCTION, 1, RETURN_VALUE, 0};
  code->setCode(runtime.newByteArrayWithAll(bytecode));
  code->setStacksize(2);

  // Execute the code and make sure we get back the result we expect
  Object* result = Thread::currentThread()->run(*code);
  ASSERT_TRUE(result->isSmallInteger());
  ASSERT_EQ(SmallInteger::cast(result)->value(), 1111);
}

TEST(ThreadTest, ExtendedArg) {
  const word numConsts = 258;
  const byte bytecode[] = {EXTENDED_ARG, 1, LOAD_CONST, 1, RETURN_VALUE, 0};

  Runtime runtime;
  HandleScope scope;
  Handle<ObjectArray> constants(&scope, runtime.newObjectArray(numConsts));

  auto zero = SmallInteger::fromWord(0);
  auto nonZero = SmallInteger::fromWord(0xDEADBEEF);
  for (word i = 0; i < numConsts - 2; i++) {
    constants->atPut(i, zero);
  }
  constants->atPut(numConsts - 1, nonZero);
  Handle<Code> code(&scope, runtime.newCode());
  code->setConsts(*constants);
  code->setCode(runtime.newByteArrayWithAll(bytecode));
  code->setStacksize(2);

  Object* result = Thread::currentThread()->run(*code);

  ASSERT_TRUE(result->isSmallInteger());
  EXPECT_EQ(SmallInteger::cast(result)->value(), 0xDEADBEEF);
}

TEST(ThreadTest, CallBuiltinPrint) {
  Runtime runtime;
  std::string output = compileAndRunToString(
      &runtime, "print(1111, 'testing 123', True, False)");
  EXPECT_EQ(output, "1111 testing 123 True False\n");
}

TEST(ThreadTest, CallBuiltinPrintKw) {
  Runtime runtime;
  std::string output =
      compileAndRunToString(&runtime, "print('testing 123', end='abc')");
  EXPECT_STREQ(output.c_str(), "testing 123abc");
}

TEST(ThreadTest, ExecuteDupTop) {
  Runtime runtime;
  HandleScope scope;

  Handle<ObjectArray> consts(&scope, runtime.newObjectArray(1));
  consts->atPut(0, SmallInteger::fromWord(1111));
  Handle<Code> code(&scope, runtime.newCode());
  code->setStacksize(2);
  code->setConsts(*consts);
  const byte bytecode[] = {LOAD_CONST, 0, DUP_TOP, 0, RETURN_VALUE, 0};
  code->setCode(runtime.newByteArrayWithAll(bytecode));

  Object* result = Thread::currentThread()->run(*code);
  ASSERT_TRUE(result->isSmallInteger());
  EXPECT_EQ(SmallInteger::cast(result)->value(), 1111);
}

TEST(ThreadTest, ExecuteDupTopTwo) {
  Runtime runtime;
  HandleScope scope;

  Handle<ObjectArray> consts(&scope, runtime.newObjectArray(2));
  consts->atPut(0, SmallInteger::fromWord(1111));
  consts->atPut(1, SmallInteger::fromWord(2222));
  Handle<Code> code(&scope, runtime.newCode());
  code->setStacksize(2);
  code->setConsts(*consts);
  const byte bytecode[] = {
      LOAD_CONST, 0, LOAD_CONST, 1, DUP_TOP_TWO, 0, RETURN_VALUE, 0};
  code->setCode(runtime.newByteArrayWithAll(bytecode));

  Object* result = Thread::currentThread()->run(*code);
  ASSERT_TRUE(result->isSmallInteger());
  EXPECT_EQ(SmallInteger::cast(result)->value(), 2222);
}

TEST(ThreadTest, ExecuteRotTwo) {
  Runtime runtime;
  HandleScope scope;

  Handle<ObjectArray> consts(&scope, runtime.newObjectArray(2));
  consts->atPut(0, SmallInteger::fromWord(1111));
  consts->atPut(1, SmallInteger::fromWord(2222));
  Handle<Code> code(&scope, runtime.newCode());
  code->setStacksize(2);
  code->setConsts(*consts);
  const byte bytecode[] = {
      LOAD_CONST, 0, LOAD_CONST, 1, ROT_TWO, 0, RETURN_VALUE, 0};
  code->setCode(runtime.newByteArrayWithAll(bytecode));

  Object* result = Thread::currentThread()->run(*code);
  ASSERT_TRUE(result->isSmallInteger());
  EXPECT_EQ(SmallInteger::cast(result)->value(), 1111);
}

TEST(ThreadTest, ExecuteRotThree) {
  Runtime runtime;
  HandleScope scope;

  Handle<ObjectArray> consts(&scope, runtime.newObjectArray(3));
  consts->atPut(0, SmallInteger::fromWord(1111));
  consts->atPut(1, SmallInteger::fromWord(2222));
  consts->atPut(2, SmallInteger::fromWord(3333));
  Handle<Code> code(&scope, runtime.newCode());
  code->setStacksize(3);
  code->setConsts(*consts);
  const byte bytecode[] = {LOAD_CONST,
                           0,
                           LOAD_CONST,
                           1,
                           LOAD_CONST,
                           2,
                           ROT_THREE,
                           0,
                           RETURN_VALUE,
                           0};
  code->setCode(runtime.newByteArrayWithAll(bytecode));

  Object* result = Thread::currentThread()->run(*code);
  ASSERT_TRUE(result->isSmallInteger());
  EXPECT_EQ(SmallInteger::cast(result)->value(), 2222);
}

TEST(ThreadTest, ExecuteJumpAbsolute) {
  Runtime runtime;
  HandleScope scope;

  Handle<ObjectArray> consts(&scope, runtime.newObjectArray(2));
  consts->atPut(0, SmallInteger::fromWord(1111));
  consts->atPut(1, SmallInteger::fromWord(2222));
  Handle<Code> code(&scope, runtime.newCode());
  code->setStacksize(2);
  code->setConsts(*consts);
  const byte bytecode[] = {
      JUMP_ABSOLUTE, 4, LOAD_CONST, 0, LOAD_CONST, 1, RETURN_VALUE, 0};
  code->setCode(runtime.newByteArrayWithAll(bytecode));

  Object* result = Thread::currentThread()->run(*code);
  ASSERT_TRUE(result->isSmallInteger());
  EXPECT_EQ(SmallInteger::cast(result)->value(), 2222);
}

TEST(ThreadTest, ExecuteJumpForward) {
  Runtime runtime;
  HandleScope scope;

  Handle<ObjectArray> consts(&scope, runtime.newObjectArray(2));
  consts->atPut(0, SmallInteger::fromWord(1111));
  consts->atPut(1, SmallInteger::fromWord(2222));
  Handle<Code> code(&scope, runtime.newCode());
  code->setStacksize(2);
  code->setConsts(*consts);
  const byte bytecode[] = {
      JUMP_FORWARD, 2, LOAD_CONST, 0, LOAD_CONST, 1, RETURN_VALUE, 0};
  code->setCode(runtime.newByteArrayWithAll(bytecode));

  Object* result = Thread::currentThread()->run(*code);
  ASSERT_TRUE(result->isSmallInteger());
  EXPECT_EQ(SmallInteger::cast(result)->value(), 2222);
}

TEST(ThreadTest, ExecuteStoreLoadFast) {
  Runtime runtime;
  HandleScope scope;

  Handle<Code> code(&scope, runtime.newCode());
  Handle<ObjectArray> consts(&scope, runtime.newObjectArray(1));
  consts->atPut(0, SmallInteger::fromWord(1111));
  code->setConsts(*consts);
  code->setNlocals(2);
  const byte bytecode[] = {
      LOAD_CONST, 0, STORE_FAST, 1, LOAD_FAST, 1, RETURN_VALUE, 0};
  code->setCode(runtime.newByteArrayWithAll(bytecode));

  Object* result = Thread::currentThread()->run(*code);
  ASSERT_TRUE(result->isSmallInteger());
  EXPECT_EQ(SmallInteger::cast(result)->value(), 1111);
}

TEST(ThreadTest, LoadGlobal) {
  Runtime runtime;
  HandleScope scope;

  Handle<Code> code(&scope, runtime.newCode());
  Handle<ObjectArray> names(&scope, runtime.newObjectArray(1));
  Handle<Object> key(&scope, runtime.newStringFromCString("foo"));
  names->atPut(0, *key);
  code->setNames(*names);

  const byte bytecode[] = {LOAD_GLOBAL, 0, RETURN_VALUE, 0};
  code->setCode(runtime.newByteArrayWithAll(bytecode));

  Thread* thread = Thread::currentThread();
  Frame* frame = thread->pushFrame(*code);

  Handle<Dictionary> globals(&scope, runtime.newDictionary());
  Handle<Dictionary> builtins(&scope, runtime.newDictionary());
  Handle<ValueCell> value_cell(&scope, runtime.newValueCell());
  value_cell->setValue(SmallInteger::fromWord(1234));
  Handle<Object> value(&scope, *value_cell);
  runtime.dictionaryAtPut(globals, key, value);
  frame->setGlobals(*globals);
  frame->setFastGlobals(runtime.computeFastGlobals(code, globals, builtins));

  Handle<Object> result(&scope, Interpreter::execute(thread, frame));
  EXPECT_EQ(*result, value_cell->value());
}

struct TestData {
  const char* name;
  const char* expected_output;
  const char* src;
  const bool death;
};

static std::string TestName(::testing::TestParamInfo<TestData> info) {
  return info.param.name;
}

TestData kFastGlobalTests[] = {{"LoadGlobal",
                                "1\n",
                                R"(
a = 1
def f():
  print(a)
f()
)",
                                false},

                               {"LoadGlobalFromBuiltin",
                                "True\n",
                                R"(
class A(): pass
a = A()
def f():
  print(isinstance(a, A))
f()
)",
                                false},

                               {"LoadGlobalUnbound",
                                ".*Unbound Globals.*",
                                R"(
def f():
  print(a)
f()
)",
                                true},

                               {"StoreGlobal",
                                "2\n2\n",
                                R"(
def f():
  global a
  a = 2
  print(a)
f()
print(a)
)",
                                false},

                               {"StoreGlobalShadowBuiltin",
                                "2\n",
                                R"(
def f():
  global isinstance
  isinstance = 2
f()
print(isinstance)
)",
                                false},

                               {"DeleteGlobal",
                                "True\nTrue\n",
                                R"(
class A(): pass
a = A()
def f():
  global isinstance
  isinstance = 1
  del isinstance
  print(isinstance(a, A))  # fallback to builtin
f()
print(isinstance(a, A))
)",
                                false},

                               {"DeleteGlobalUnbound",
                                ".*Unbound Globals.*",
                                R"(
def f():
  global a
  del a
f()
)",
                                true},

                               {"DeleteGlobalBuiltinUnbound",
                                ".*Unbound Globals.*",
                                R"(
def f():
  global isinstance
  del isinstance
f()
)",
                                true}

};

class GlobalsTest : public ::testing::TestWithParam<TestData> {};

TEST_P(GlobalsTest, FastGlobal) {
  Runtime runtime;
  TestData data = GetParam();
  if (data.death) {
    EXPECT_DEATH(
        compileAndRunToString(&runtime, data.src), data.expected_output);
  } else {
    std::string output = compileAndRunToString(&runtime, data.src);
    EXPECT_EQ(output, data.expected_output);
  }
}

INSTANTIATE_TEST_CASE_P(
    FastGlobal,
    GlobalsTest,
    ::testing::ValuesIn(kFastGlobalTests),
    TestName);

TEST(ThreadTest, StoreGlobalCreateValueCell) {
  Runtime runtime;
  HandleScope scope;

  Handle<Code> code(&scope, runtime.newCode());

  Handle<ObjectArray> consts(&scope, runtime.newObjectArray(1));
  consts->atPut(0, SmallInteger::fromWord(42));
  code->setConsts(*consts);

  Handle<ObjectArray> names(&scope, runtime.newObjectArray(1));
  Handle<Object> key(&scope, runtime.newStringFromCString("foo"));
  names->atPut(0, *key);
  code->setNames(*names);

  const byte bytecode[] = {
      LOAD_CONST, 0, STORE_GLOBAL, 0, LOAD_GLOBAL, 0, RETURN_VALUE, 0};
  code->setCode(runtime.newByteArrayWithAll(bytecode));

  Thread* thread = Thread::currentThread();
  Frame* frame = thread->pushFrame(*code);

  Handle<Dictionary> globals(&scope, runtime.newDictionary());
  Handle<Dictionary> builtins(&scope, runtime.newDictionary());
  frame->setGlobals(*globals);
  frame->setFastGlobals(runtime.computeFastGlobals(code, globals, builtins));

  Handle<Object> result(&scope, Interpreter::execute(thread, frame));

  Handle<Object> value(&scope, runtime.dictionaryAt(globals, key));
  ASSERT_TRUE(value->isValueCell());
  EXPECT_EQ(*result, ValueCell::cast(*value)->value());
}

TEST(ThreadTest, StoreGlobalReuseValueCell) {
  Runtime runtime;
  HandleScope scope;

  Handle<Code> code(&scope, runtime.newCode());

  Handle<ObjectArray> consts(&scope, runtime.newObjectArray(1));
  consts->atPut(0, SmallInteger::fromWord(42));
  code->setConsts(*consts);

  Handle<ObjectArray> names(&scope, runtime.newObjectArray(1));
  Handle<Object> key(&scope, runtime.newStringFromCString("foo"));
  names->atPut(0, *key);
  code->setNames(*names);

  const byte bytecode[] = {
      LOAD_CONST, 0, STORE_GLOBAL, 0, LOAD_GLOBAL, 0, RETURN_VALUE, 0};
  code->setCode(runtime.newByteArrayWithAll(bytecode));

  Thread* thread = Thread::currentThread();
  Frame* frame = thread->pushFrame(*code);

  Handle<ValueCell> value_cell1(&scope, runtime.newValueCell());
  value_cell1->setValue(SmallInteger::fromWord(99));

  Handle<Dictionary> globals(&scope, runtime.newDictionary());
  Handle<Dictionary> builtins(&scope, runtime.newDictionary());
  Handle<Object> value(&scope, *value_cell1);
  runtime.dictionaryAtPut(globals, key, value);
  frame->setGlobals(*globals);
  frame->setFastGlobals(runtime.computeFastGlobals(code, globals, builtins));

  Handle<Object> result(&scope, Interpreter::execute(thread, frame));

  Handle<Object> value_cell2(&scope, runtime.dictionaryAt(globals, key));
  ASSERT_TRUE(value_cell2->isValueCell());
  EXPECT_EQ(*value_cell2, *value_cell1);
  EXPECT_EQ(SmallInteger::fromWord(42), value_cell1->value());
}

TEST(ThreadTest, StoreNameCreateValueCell) {
  Runtime runtime;
  HandleScope scope;

  Handle<Code> code(&scope, runtime.newCode());

  Handle<ObjectArray> consts(&scope, runtime.newObjectArray(1));
  consts->atPut(0, SmallInteger::fromWord(42));
  code->setConsts(*consts);

  Handle<ObjectArray> names(&scope, runtime.newObjectArray(1));
  Handle<Object> key(&scope, runtime.newStringFromCString("foo"));
  names->atPut(0, *key);
  code->setNames(*names);

  const byte bytecode[] = {
      LOAD_CONST, 0, STORE_NAME, 0, LOAD_NAME, 0, RETURN_VALUE, 0};
  code->setCode(runtime.newByteArrayWithAll(bytecode));

  Thread* thread = Thread::currentThread();
  Frame* frame = thread->pushFrame(*code);

  Handle<Dictionary> implicit_globals(&scope, runtime.newDictionary());
  Handle<Dictionary> builtins(&scope, runtime.newDictionary());
  frame->setImplicitGlobals(*implicit_globals);
  frame->setFastGlobals(
      runtime.computeFastGlobals(code, implicit_globals, builtins));

  Handle<Object> result(&scope, Interpreter::execute(thread, frame));

  Handle<Object> value(&scope, runtime.dictionaryAt(implicit_globals, key));
  ASSERT_TRUE(value->isValueCell());
  EXPECT_EQ(*result, ValueCell::cast(*value)->value());
}

TEST(ThreadTest, MakeFunction) {
  Runtime runtime;
  HandleScope scope;

  Handle<Code> module(&scope, runtime.newCode());

  Handle<ObjectArray> consts(&scope, runtime.newObjectArray(3));
  Handle<Code> code(&scope, runtime.newCode());
  consts->atPut(0, *code);
  Handle<Object> key(&scope, runtime.newStringFromCString("hello"));
  consts->atPut(1, *key);
  consts->atPut(2, None::object());
  module->setConsts(*consts);

  Handle<ObjectArray> names(&scope, runtime.newObjectArray(1));
  names->atPut(0, runtime.newStringFromCString("hello"));
  module->setNames(*names);

  const byte bc[] = {LOAD_CONST,
                     0,
                     LOAD_CONST,
                     1,
                     MAKE_FUNCTION,
                     0,
                     STORE_NAME,
                     0,
                     LOAD_CONST,
                     2,
                     RETURN_VALUE,
                     0};
  module->setCode(runtime.newByteArrayWithAll(bc));
  code->setCode(runtime.newByteArrayWithAll(bc));
  code->setNames(*names);

  Thread* thread = Thread::currentThread();
  Frame* frame = thread->pushFrame(*module);

  Handle<Dictionary> implicit_globals(&scope, runtime.newDictionary());
  Handle<Dictionary> globals(&scope, runtime.newDictionary());
  Handle<Dictionary> builtins(&scope, runtime.newDictionary());
  frame->setGlobals(*globals);
  frame->setBuiltins(*builtins);
  frame->setImplicitGlobals(*implicit_globals);

  Handle<Object> result(&scope, Interpreter::execute(thread, frame));

  Handle<Object> value(&scope, runtime.dictionaryAt(implicit_globals, key));
  ASSERT_TRUE(value->isValueCell());
  ASSERT_TRUE(ValueCell::cast(*value)->value()->isFunction());

  Handle<Function> function(&scope, ValueCell::cast(*value)->value());
  EXPECT_EQ(function->code(), consts->at(0));
  EXPECT_EQ(function->name(), consts->at(1));
  EXPECT_EQ(function->entry(), &interpreterTrampoline);
}

TEST(ThreadTest, BuildList) {
  Runtime runtime;
  HandleScope scope;

  Handle<Code> code(&scope, runtime.newCode());

  Handle<ObjectArray> consts(&scope, runtime.newObjectArray(3));
  consts->atPut(0, SmallInteger::fromWord(111));
  consts->atPut(1, runtime.newStringFromCString("qqq"));
  consts->atPut(2, None::object());
  code->setConsts(*consts);

  const byte bc[] = {LOAD_CONST,
                     0,
                     LOAD_CONST,
                     1,
                     LOAD_CONST,
                     2,
                     BUILD_LIST,
                     3,
                     RETURN_VALUE,
                     0};
  code->setCode(runtime.newByteArrayWithAll(bc));

  Object* result = Thread::currentThread()->run(*code);
  ASSERT_TRUE(result->isList());

  List* list = List::cast(result);
  EXPECT_EQ(list->capacity(), 3);

  ASSERT_TRUE(list->at(0)->isSmallInteger());
  EXPECT_EQ(SmallInteger::cast(list->at(0))->value(), 111);

  ASSERT_TRUE(list->at(1)->isSmallString());
  EXPECT_EQ(list->at(1), SmallString::fromCString("qqq"));
  EXPECT_EQ(list->at(2), None::object());
}

TEST(ThreadTest, BuildSetEmpty) {
  Runtime runtime;
  HandleScope scope;

  Handle<Code> code(&scope, runtime.newCode());
  const byte bc[] = {BUILD_SET, 0, RETURN_VALUE, 0};
  code->setCode(runtime.newByteArrayWithAll(bc));

  Object* result = Thread::currentThread()->run(*code);
  ASSERT_TRUE(result->isSet());

  Handle<Set> set(&scope, result);
  EXPECT_EQ(set->numItems(), 0);
}

TEST(ThreadTest, BuildSetWithOneItem) {
  Runtime runtime;
  HandleScope scope;
  Handle<Code> code(&scope, runtime.newCode());

  Handle<ObjectArray> consts(&scope, runtime.newObjectArray(2));
  Handle<Object> smi(&scope, SmallInteger::fromWord(111));
  consts->atPut(0, *smi);
  consts->atPut(1, *smi); // dup
  code->setConsts(*consts);

  const byte bc[] = {
      LOAD_CONST, 0, LOAD_CONST, 1, BUILD_SET, 2, RETURN_VALUE, 0};
  code->setCode(runtime.newByteArrayWithAll(bc));

  Object* result = Thread::currentThread()->run(*code);
  ASSERT_TRUE(result->isSet());

  Handle<Set> set(&scope, result);
  EXPECT_EQ(set->numItems(), 1);

  EXPECT_TRUE(runtime.setIncludes(set, smi));
}

TEST(ThreadTest, BuildSet) {
  Runtime runtime;
  HandleScope scope;
  Handle<Code> code(&scope, runtime.newCode());

  Handle<ObjectArray> consts(&scope, runtime.newObjectArray(4));

  Handle<Object> smi(&scope, SmallInteger::fromWord(111));
  consts->atPut(0, *smi);
  consts->atPut(1, *smi); // dup

  Handle<Object> str(&scope, runtime.newStringFromCString("qqq"));
  consts->atPut(2, *str);

  Handle<Object> none(&scope, None::object());
  consts->atPut(3, *none);

  code->setConsts(*consts);

  const byte bc[] = {LOAD_CONST,
                     0,
                     LOAD_CONST,
                     1,
                     LOAD_CONST,
                     2,
                     LOAD_CONST,
                     3,
                     BUILD_SET,
                     4,
                     RETURN_VALUE,
                     0};
  code->setCode(runtime.newByteArrayWithAll(bc));

  Object* result = Thread::currentThread()->run(*code);
  ASSERT_TRUE(result->isSet());

  Handle<Set> set(&scope, result);
  EXPECT_EQ(set->numItems(), 3);

  EXPECT_TRUE(runtime.setIncludes(set, smi));
  EXPECT_TRUE(runtime.setIncludes(set, str));
  EXPECT_TRUE(runtime.setIncludes(set, none));
}

TEST(ThreadTest, SetupLoop) {
  Runtime runtime;
  HandleScope scope;

  const byte bc[] = {SETUP_LOOP, 100, RETURN_VALUE, 0};
  Handle<Code> code(&scope, runtime.newCode());
  code->setCode(runtime.newByteArrayWithAll(bc));
  code->setStacksize(3);

  // Create a frame with three items on the stack
  auto thread = Thread::currentThread();
  auto frame = thread->pushFrame(*code);
  Object** sp = frame->valueStackTop();
  *--sp = SmallInteger::fromWord(1111);
  *--sp = SmallInteger::fromWord(2222);
  *--sp = SmallInteger::fromWord(3333);
  frame->setValueStackTop(sp);

  Interpreter::execute(thread, frame);

  // SETUP_LOOP should have pushed an entry onto the block stack with a
  // stack depth of 3
  TryBlock block = frame->blockStack()->pop();
  EXPECT_EQ(block.kind(), Bytecode::SETUP_LOOP);
  EXPECT_EQ(block.handler(), 102);
  EXPECT_EQ(block.level(), 3);
}

TEST(ThreadTest, PopBlock) {
  Runtime runtime;
  HandleScope scope;

  const byte bc[] = {POP_BLOCK, 0, RETURN_VALUE, 0};
  Handle<Code> code(&scope, runtime.newCode());
  code->setCode(runtime.newByteArrayWithAll(bc));
  code->setStacksize(3);

  // Create a frame with three items on the stack
  auto thread = Thread::currentThread();
  auto frame = thread->pushFrame(*code);
  Object** sp = frame->valueStackTop();
  *--sp = SmallInteger::fromWord(1111);
  *--sp = SmallInteger::fromWord(2222);
  *--sp = SmallInteger::fromWord(3333);
  frame->setValueStackTop(sp);

  // Push an entry onto the block stack. When popped, this should set the stack
  // pointer to point to the bottom most element on the stack.
  frame->blockStack()->push(TryBlock(Bytecode::SETUP_LOOP, 0, 1));

  Object* result = Interpreter::execute(thread, frame);

  // The RETURN_VALUE instruction should return bottom most item from the stack,
  // assuming that POP_BLOCK worked correctly.
  ASSERT_TRUE(result->isSmallInteger());
  EXPECT_EQ(SmallInteger::cast(result)->value(), 1111);
}

TEST(ThreadTest, PopJumpIfFalse) {
  Runtime runtime;
  HandleScope scope;

  Handle<Code> code(&scope, runtime.newCode());
  Handle<ObjectArray> consts(&scope, runtime.newObjectArray(3));
  consts->atPut(0, Boolean::fromBool(true));
  consts->atPut(1, SmallInteger::fromWord(1111));
  consts->atPut(2, SmallInteger::fromWord(2222));
  code->setConsts(*consts);
  // Bytecode for the snippet:
  //   if x:
  //     return 1111
  //   return 2222
  const byte bc[] = {LOAD_CONST,
                     0,
                     POP_JUMP_IF_FALSE,
                     8,
                     LOAD_CONST,
                     1,
                     RETURN_VALUE,
                     0,
                     LOAD_CONST,
                     2,
                     RETURN_VALUE,
                     0};
  code->setCode(runtime.newByteArrayWithAll(bc));

  // Test when the condition evaluates to a truthy value
  Object* result = Thread::currentThread()->run(*code);
  ASSERT_TRUE(result->isSmallInteger());
  EXPECT_EQ(SmallInteger::cast(result)->value(), 1111);

  // Test when the condition evaluates to a falsey value
  consts->atPut(0, Boolean::fromBool(false));
  result = Thread::currentThread()->run(*code);
  ASSERT_TRUE(result->isSmallInteger());
  EXPECT_EQ(SmallInteger::cast(result)->value(), 2222);
}

TEST(ThreadTest, PopJumpIfTrue) {
  Runtime runtime;
  HandleScope scope;

  Handle<Code> code(&scope, runtime.newCode());
  Handle<ObjectArray> consts(&scope, runtime.newObjectArray(3));
  consts->atPut(0, Boolean::fromBool(false));
  consts->atPut(1, SmallInteger::fromWord(1111));
  consts->atPut(2, SmallInteger::fromWord(2222));
  code->setConsts(*consts);
  // Bytecode for the snippet:
  //   if not x:
  //     return 1111
  //   return 2222
  const byte bc[] = {LOAD_CONST,
                     0,
                     POP_JUMP_IF_TRUE,
                     8,
                     LOAD_CONST,
                     1,
                     RETURN_VALUE,
                     0,
                     LOAD_CONST,
                     2,
                     RETURN_VALUE,
                     0};
  code->setCode(runtime.newByteArrayWithAll(bc));

  // Test when the condition evaluates to a falsey value
  Object* result = Thread::currentThread()->run(*code);
  ASSERT_TRUE(result->isSmallInteger());
  EXPECT_EQ(SmallInteger::cast(result)->value(), 1111);

  // Test when the condition evaluates to a truthy value
  consts->atPut(0, Boolean::fromBool(true));
  result = Thread::currentThread()->run(*code);
  ASSERT_TRUE(result->isSmallInteger());
  EXPECT_EQ(SmallInteger::cast(result)->value(), 2222);
}

TEST(ThreadTest, JumpIfFalseOrPop) {
  Runtime runtime;
  HandleScope scope;

  Handle<Code> code(&scope, runtime.newCode());
  Handle<ObjectArray> consts(&scope, runtime.newObjectArray(2));
  consts->atPut(0, Boolean::fromBool(false));
  consts->atPut(1, SmallInteger::fromWord(1111));
  code->setConsts(*consts);
  const byte bc[] = {
      LOAD_CONST, 0, JUMP_IF_FALSE_OR_POP, 6, LOAD_CONST, 1, RETURN_VALUE, 0};
  code->setCode(runtime.newByteArrayWithAll(bc));

  // If the condition is false, we should return the top of the stack, which is
  // the condition itself
  Object* result = Thread::currentThread()->run(*code);
  ASSERT_TRUE(result->isBoolean());
  EXPECT_FALSE(Boolean::cast(result)->value());

  // If the condition is true, we should pop the top of the stack (the
  // condition) and continue execution. In our case that loads a const and
  // returns it.
  consts->atPut(0, Boolean::fromBool(true));
  result = Thread::currentThread()->run(*code);
  ASSERT_TRUE(result->isSmallInteger());
  EXPECT_EQ(SmallInteger::cast(result)->value(), 1111);
}

TEST(ThreadTest, JumpIfTrueOrPop) {
  Runtime runtime;
  HandleScope scope;

  Handle<Code> code(&scope, runtime.newCode());
  Handle<ObjectArray> consts(&scope, runtime.newObjectArray(2));
  consts->atPut(0, Boolean::fromBool(true));
  consts->atPut(1, SmallInteger::fromWord(1111));
  code->setConsts(*consts);
  const byte bc[] = {
      LOAD_CONST, 0, JUMP_IF_TRUE_OR_POP, 6, LOAD_CONST, 1, RETURN_VALUE, 0};
  code->setCode(runtime.newByteArrayWithAll(bc));

  // If the condition is true, we should return the top of the stack, which is
  // the condition itself
  Object* result = Thread::currentThread()->run(*code);
  ASSERT_TRUE(result->isBoolean());
  EXPECT_TRUE(Boolean::cast(result)->value());

  // If the condition is false, we should pop the top of the stack (the
  // condition) and continue execution. In our case that loads a const and
  // returns it.
  consts->atPut(0, Boolean::fromBool(false));
  result = Thread::currentThread()->run(*code);
  ASSERT_TRUE(result->isSmallInteger());
  EXPECT_EQ(SmallInteger::cast(result)->value(), 1111);
}

TEST(ThreadTest, UnaryNot) {
  Runtime runtime;
  HandleScope scope;

  Handle<Code> code(&scope, runtime.newCode());
  Handle<ObjectArray> consts(&scope, runtime.newObjectArray(1));
  consts->atPut(0, Boolean::fromBool(true));
  code->setConsts(*consts);
  // Bytecode for the snippet:
  //     return not x
  const byte bc[] = {LOAD_CONST, 0, UNARY_NOT, 0, RETURN_VALUE, 0};
  code->setCode(runtime.newByteArrayWithAll(bc));

  // If the condition is true, we should return false
  Object* result = Thread::currentThread()->run(*code);
  ASSERT_TRUE(result->isBoolean());
  EXPECT_FALSE(Boolean::cast(result)->value());

  // If the condition is false, we should return true
  consts->atPut(0, Boolean::fromBool(false));
  result = Thread::currentThread()->run(*code);
  ASSERT_TRUE(result->isBoolean());
  EXPECT_TRUE(Boolean::cast(result)->value());
}

static Dictionary* getMainModuleDict(Runtime* runtime) {
  HandleScope scope;
  Handle<Module> mod(&scope, findModule(runtime, "__main__"));
  EXPECT_TRUE(mod->isModule());

  Handle<Dictionary> dict(&scope, mod->dictionary());
  EXPECT_TRUE(dict->isDictionary());
  return *dict;
}

TEST(ThreadTest, LoadBuildClassEmptyClass) {
  Runtime runtime;
  HandleScope scope;

  const char* src = R"(
class C:
  pass
)";
  std::unique_ptr<char[]> buffer(Runtime::compile(src));

  Object* result = runtime.run(buffer.get());
  ASSERT_EQ(result, None::object()); // returns None

  Handle<Dictionary> dict(&scope, getMainModuleDict(&runtime));

  Handle<Object> key(&scope, runtime.newStringFromCString("C"));
  Handle<Object> value(&scope, runtime.dictionaryAt(dict, key));
  ASSERT_TRUE(value->isValueCell());

  Handle<Class> cls(&scope, ValueCell::cast(*value)->value());
  ASSERT_TRUE(cls->name()->isSmallString());
  EXPECT_EQ(cls->name(), SmallString::fromCString("C"));

  Handle<ObjectArray> mro(&scope, cls->mro());
  EXPECT_EQ(mro->length(), 2);
  EXPECT_EQ(mro->at(0), *cls);
  EXPECT_EQ(mro->at(1), runtime.classAt(IntrinsicLayoutId::kObject));
}

TEST(ThreadTest, LoadBuildClassClassWithInit) {
  Runtime runtime;
  HandleScope scope;

  const char* src = R"(
class C:
  def __init__(self):
    pass
)";

  std::unique_ptr<char[]> buffer(Runtime::compile(src));

  Object* result = runtime.run(buffer.get());
  ASSERT_EQ(result, None::object()); // returns None

  Handle<Module> mod(&scope, findModule(&runtime, "__main__"));
  ASSERT_TRUE(mod->isModule());

  Handle<Dictionary> mod_dict(&scope, mod->dictionary());
  ASSERT_TRUE(mod_dict->isDictionary());

  // Check for the class name in the module dictionary
  Handle<Object> cls_name(&scope, runtime.newStringFromCString("C"));
  Handle<Object> value(&scope, runtime.dictionaryAt(mod_dict, cls_name));
  ASSERT_TRUE(value->isValueCell());
  Handle<Class> cls(&scope, ValueCell::cast(*value)->value());

  // Check class MRO
  Handle<ObjectArray> mro(&scope, cls->mro());
  EXPECT_EQ(mro->length(), 2);
  EXPECT_EQ(mro->at(0), *cls);
  EXPECT_EQ(mro->at(1), runtime.classAt(IntrinsicLayoutId::kObject));

  // Check class name
  ASSERT_TRUE(cls->name()->isSmallString());
  EXPECT_EQ(cls->name(), SmallString::fromCString("C"));

  Handle<Dictionary> cls_dict(&scope, cls->dictionary());
  ASSERT_TRUE(cls_dict->isDictionary());

  // Check for the __init__ method name in the dictionary
  Handle<Object> meth_name(&scope, runtime.symbols()->DunderInit());
  ASSERT_TRUE(runtime.dictionaryIncludes(cls_dict, meth_name));
  value = runtime.dictionaryAt(cls_dict, meth_name);
  ASSERT_TRUE(value->isValueCell());
  ASSERT_TRUE(ValueCell::cast(*value)->value()->isFunction());
}

static Object* nativeExceptionTest(Thread* thread, Frame*, word) {
  HandleScope scope;
  Handle<String> msg(
      &scope,
      String::cast(thread->runtime()->newStringFromCString("test exception")));
  thread->throwRuntimeError(*msg);
  return Error::object();
}

TEST(ThreadTest, NativeExceptions) {
  Runtime runtime;
  HandleScope scope;

  Handle<Function> fn(
      &scope,
      runtime.newBuiltinFunction(
          nativeTrampoline<nativeExceptionTest>,
          nativeTrampoline<unimplementedTrampoline>));

  Handle<Code> code(&scope, runtime.newCode());
  Handle<ObjectArray> consts(&scope, runtime.newObjectArray(1));
  consts->atPut(0, *fn);
  code->setConsts(*consts);

  // Call the native function and assert that it causes program termination due
  // to throwing an exception.
  const byte bytecode[] = {LOAD_CONST, 0, CALL_FUNCTION, 0, RETURN_VALUE, 0};
  code->setCode(runtime.newByteArrayWithAll(bytecode));
  code->setStacksize(1);

  ASSERT_DEATH(
      Thread::currentThread()->run(*code),
      "aborting due to pending exception: test exception");
}

// MRO tests

static String* className(Object* obj) {
  HandleScope scope;
  Handle<Class> cls(&scope, Class::cast(obj));
  Handle<String> name(&scope, String::cast(cls->name()));
  return *name;
}

static Object*
getMro(Runtime* runtime, const char* src, const char* desired_class) {
  HandleScope scope;

  std::unique_ptr<char[]> buffer(Runtime::compile(src));
  Handle<Object> result(&scope, runtime->run(buffer.get()));

  Handle<Dictionary> mod_dict(&scope, getMainModuleDict(runtime));
  Handle<Object> class_name(
      &scope, runtime->newStringFromCString(desired_class));

  Handle<Object> value(&scope, runtime->dictionaryAt(mod_dict, class_name));
  Handle<Class> cls(&scope, ValueCell::cast(*value)->value());

  return cls->mro();
}

TEST(ThreadTest, LoadBuildClassVerifyMro) {
  Runtime runtime;
  HandleScope scope;

  const char* src = R"(
class A: pass
class B: pass
class C(A,B): pass
)";

  Handle<ObjectArray> mro(&scope, getMro(&runtime, src, "C"));
  EXPECT_EQ(mro->length(), 4);
  EXPECT_PYSTRING_EQ(className(mro->at(0)), "C");
  EXPECT_PYSTRING_EQ(className(mro->at(1)), "A");
  EXPECT_PYSTRING_EQ(className(mro->at(2)), "B");
  EXPECT_PYSTRING_EQ(className(mro->at(3)), "object");
}

TEST(ThreadTest, LoadBuildClassVerifyMroInheritance) {
  Runtime runtime;
  HandleScope scope;

  const char* src = R"(
class A: pass
class B(A): pass
class C(B): pass
)";

  Handle<ObjectArray> mro(&scope, getMro(&runtime, src, "C"));
  EXPECT_EQ(mro->length(), 4);
  EXPECT_PYSTRING_EQ(className(mro->at(0)), "C");
  EXPECT_PYSTRING_EQ(className(mro->at(1)), "B");
  EXPECT_PYSTRING_EQ(className(mro->at(2)), "A");
  EXPECT_PYSTRING_EQ(className(mro->at(3)), "object");
}

TEST(ThreadTest, LoadBuildClassVerifyMroMultiInheritance) {
  Runtime runtime;
  HandleScope scope;

  const char* src = R"(
class A: pass
class B(A): pass
class C: pass
class D(B,C): pass
)";

  Handle<ObjectArray> mro(&scope, getMro(&runtime, src, "D"));
  EXPECT_EQ(mro->length(), 5);
  EXPECT_PYSTRING_EQ(className(mro->at(0)), "D");
  EXPECT_PYSTRING_EQ(className(mro->at(1)), "B");
  EXPECT_PYSTRING_EQ(className(mro->at(2)), "A");
  EXPECT_PYSTRING_EQ(className(mro->at(3)), "C");
  EXPECT_PYSTRING_EQ(className(mro->at(4)), "object");
}

TEST(ThreadTest, LoadBuildClassVerifyMroDiamond) {
  Runtime runtime;
  HandleScope scope;

  const char* src = R"(
class A: pass
class B(A): pass
class C(A): pass
class D(B,C): pass
)";

  Handle<ObjectArray> mro(&scope, getMro(&runtime, src, "D"));
  EXPECT_EQ(mro->length(), 5);
  EXPECT_PYSTRING_EQ(className(mro->at(0)), "D");
  EXPECT_PYSTRING_EQ(className(mro->at(1)), "B");
  EXPECT_PYSTRING_EQ(className(mro->at(2)), "C");
  EXPECT_PYSTRING_EQ(className(mro->at(3)), "A");
  EXPECT_PYSTRING_EQ(className(mro->at(4)), "object");
}

TEST(ThreadTest, LoadBuildClassVerifyMroError) {
  Runtime runtime;
  HandleScope scope;

  const char* src = R"(
class A: pass
class B(A): pass
class C(A, B): pass
)";

  std::unique_ptr<char[]> buffer(Runtime::compile(src));
  EXPECT_DEATH(runtime.run(buffer.get()), "consistent method resolution order");
}

// iteration

TEST(ThreadTest, IteratePrint) {
  Runtime runtime;
  HandleScope scope;

  const char* src = R"(
for i in range(3):
  print(i)
for i in range(3,6):
  print(i)
for i in range(6,12,2):
  print(i)
for i in range(6,3,-1):
  print(i)
for i in range(42,0,1):
  print(i)
for i in range(42,100,-1):
  print(i)
)";

  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "0\n1\n2\n3\n4\n5\n6\n8\n10\n6\n5\n4\n");
}

TEST(ThreadTest, BinaryOverflowCheck) {
  Runtime runtime;
  HandleScope scope;

  const char* mul_src = R"(
a = 268435456
a = a * a * a
)";
  (void)mul_src;

  // Overflows in the multiplication itself.
  EXPECT_DEBUG_ONLY_DEATH(
      compileAndRunToString(&runtime, mul_src), "small integer overflow");

  const char* add_src = R"(
a = 1048576
a *= 2048
a = a * a

a += a
)";
  (void)add_src;

  // No overflow per se, but result too large to store in a SmallInteger
  EXPECT_DEBUG_ONLY_DEATH(
      compileAndRunToString(&runtime, add_src), "SmallInteger::isValid");
}

TEST(ThreadTest, BinaryOps) {
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

TEST(ThreadTest, InplaceOps) {
  Runtime runtime;
  HandleScope scope;

  const char* src = R"(
a = 2
print(a)
a += 3
print(a)
a *= 5
print(a)
a //= 2
print(a)
a %= 5
print(a)
a -= -6
print(a)
a ^= 9
print(a)
)";

  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, R"(2
5
25
12
2
8
1
)");
}

TestData kManipulateLocalsTests[] = {
    // Load an argument when no local variables are present
    {"LoadSingleArg",
     "1\n",
     R"(
def test(x):
  print(x)
test(1)
)",
     false},

    // Load and store an argument when no local variables are present
    {"LoadStoreSingleArg",
     "1\n2\n",
     R"(
def test(x):
  print(x)
  x = 2
  print(x)
test(1)
)",
     false},

    // Load multiple arguments when no local variables are present
    {"LoadManyArgs",
     "1 2 3\n",
     R"(
def test(x, y, z):
  print(x, y, z)
test(1, 2, 3)
)",
     false},

    // Load/store multiple arguments when no local variables are present
    {"LoadStoreManyArgs",
     "1 2 3\n3 2 1\n",
     R"(
def test(x, y, z):
  print(x, y, z)
  x = 3
  z = 1
  print(x, y, z)
test(1, 2, 3)
)",
     false},

    // Load a single local variable when no arguments are present
    {"LoadSingleLocalVar",
     "1\n",
     R"(
def test():
  x = 1
  print(x)
test()
)",
     false},

    // Load multiple local variables when no arguments are present
    {"LoadManyLocalVars",
     "1 2 3\n",
     R"(
def test():
  x = 1
  y = 2
  z = 3
  print(x, y, z)
test()
)",
     false},

    // Mixed local var and arg usage
    {"MixedLocals",
     "1 2 3\n3 2 1\n",
     R"(
def test(x, y):
  z = 3
  print(x, y, z)
  x = z
  z = 1
  print(x, y, z)
test(1, 2)
)",
     false},
};

class LocalsTest : public ::testing::TestWithParam<TestData> {};

TEST_P(LocalsTest, ManipulateLocals) {
  Runtime runtime;
  TestData data = GetParam();
  std::string output = compileAndRunToString(&runtime, data.src);
  EXPECT_EQ(output, data.expected_output);
}

INSTANTIATE_TEST_CASE_P(
    ManipulateLocals,
    LocalsTest,
    ::testing::ValuesIn(kManipulateLocalsTests),
    TestName);

TEST(ThreadTest, BuiltinChr) {
  Runtime runtime;
  std::string result = compileAndRunToString(&runtime, "print(chr(65))");
  EXPECT_EQ(result, "A\n");
  std::unique_ptr<char[]> buffer1(Runtime::compile("print(chr(1,2))"));
  ASSERT_DEATH(
      runtime.run(buffer1.get()),
      "aborting due to pending exception: Unexpected 1 argumment in 'chr'");
  std::unique_ptr<char[]> buffer2(Runtime::compile("print(chr('A'))"));
  ASSERT_DEATH(
      runtime.run(buffer2.get()),
      "aborting due to pending exception: Unsupported type in builtin 'chr'");
}

TEST(ThreadTest, BuiltinLen) {
  Runtime runtime;
  std::string result = compileAndRunToString(&runtime, "print(len([1,2,3]))");
  EXPECT_EQ(result, "3\n");
  std::unique_ptr<char[]> buffer1(Runtime::compile("print(len(1,2))"));
  ASSERT_DEATH(
      runtime.run(buffer1.get()),
      "aborting due to pending exception: "
      "len\\(\\) takes exactly one argument");
  std::unique_ptr<char[]> buffer2(Runtime::compile("print(len(1))"));
  ASSERT_DEATH(
      runtime.run(buffer2.get()),
      "aborting due to pending exception: "
      "Unsupported type in builtin 'len'");
}

TEST(ThreadTest, BuiltinOrd) {
  Runtime runtime;
  std::string result = compileAndRunToString(&runtime, "print(ord('A'))");
  EXPECT_EQ(result, "65\n");
  ASSERT_DEATH(
      compileAndRunToString(&runtime, "print(ord(1,2))"),
      "aborting due to pending exception: Unexpected 1 argumment in 'ord'");
  ASSERT_DEATH(
      compileAndRunToString(&runtime, "print(ord(1))"),
      "aborting due to pending exception: Unsupported type in builtin 'ord'");
}

TEST(ThreadTest, CallBoundMethod) {
  Runtime runtime;

  const char* src = R"(
def func(self):
  print(self)

def test(callable):
  return callable()
)";
  compileAndRunToString(&runtime, src);

  HandleScope scope;
  Handle<Module> module(&scope, findModule(&runtime, "__main__"));
  Handle<Object> function(&scope, findInModule(&runtime, module, "func"));
  ASSERT_TRUE(function->isFunction());

  Handle<Object> self(&scope, SmallInteger::fromWord(1111));
  Handle<BoundMethod> method(&scope, runtime.newBoundMethod(function, self));

  Handle<Object> test(&scope, findInModule(&runtime, module, "test"));
  ASSERT_TRUE(test->isFunction());
  Handle<Function> func(&scope, *test);

  Handle<ObjectArray> args(&scope, runtime.newObjectArray(1));
  args->atPut(0, *method);

  std::string output = callFunctionToString(func, args);
  EXPECT_EQ(output, "1111\n");
}

TEST(ThreadTest, CallBoundMethodWithArgs) {
  Runtime runtime;

  const char* src = R"(
def func(self, a, b):
  print(self, a, b)

def test(callable):
  return callable(2222, 3333)
)";
  compileAndRunToString(&runtime, src);

  HandleScope scope;
  Handle<Module> module(&scope, findModule(&runtime, "__main__"));
  Handle<Object> function(&scope, findInModule(&runtime, module, "func"));
  ASSERT_TRUE(function->isFunction());

  Handle<Object> self(&scope, SmallInteger::fromWord(1111));
  Handle<BoundMethod> method(&scope, runtime.newBoundMethod(function, self));

  Handle<Object> test(&scope, findInModule(&runtime, module, "test"));
  ASSERT_TRUE(test->isFunction());
  Handle<Function> func(&scope, *test);

  Handle<ObjectArray> args(&scope, runtime.newObjectArray(1));
  args->atPut(0, *method);

  std::string output = callFunctionToString(func, args);
  EXPECT_EQ(output, "1111 2222 3333\n");
}

TEST(ThreadTest, CallDefaultArgs) {
  Runtime runtime;
  HandleScope scope;

  const char* src = R"(
def foo(a=1, b=2, c=3):
  print(a, b, c)

print()
foo(33, 22, 11)
foo()
foo(1001)
foo(1001, 1002)
foo(1001, 1002, 1003)
)";

  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, R"(
33 22 11
1 2 3
1001 2 3
1001 1002 3
1001 1002 1003
)");
}

TEST(ThreadTest, CallMethodMixPosDefaultArgs) {
  const char* src = R"(
def foo(a, b=2):
  print(a, b)
foo(1)
)";

  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "1 2\n");
}

TEST(ThreadTest, CallBoundMethodMixed) {
  const char* src = R"(
class R:
  def __init__(self, a, b=2):
    print(a, b)
a = R(9)
)";

  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "9 2\n");
}

TEST(ThreadTest, RaiseVarargs) {
  Runtime runtime;
  ASSERT_DEATH(
      compileAndRunToString(&runtime, "raise 1"),
      "unimplemented: bytecode 'RAISE_VARARGS'");
}

TEST(ThreadTest, BuiltinIsinstance) {
  Runtime runtime;
  HandleScope scope;

  // Only accepts 2 arguments
  EXPECT_DEATH(
      compileAndRunToString(&runtime, "print(isinstance(1, 1, 1))"),
      "aborting due to pending exception: isinstance expected 2 arguments");

  // 2nd argument must be a type
  EXPECT_DEATH(
      compileAndRunToString(&runtime, "print(isinstance(1, 1))"),
      "aborting due to pending exception: isinstance arg 2 must be a type");

  const char* src = R"(
class A: pass
class B(A): pass
class C(A): pass
class D(C, B): pass

def test(a, b):
  print(isinstance(a, b))
)";
  compileAndRunToString(&runtime, src);

  // We can move these tests into the python code above once we can
  // call classes.
  Object* object = findModule(&runtime, "__main__");
  ASSERT_TRUE(object->isModule());
  Handle<Module> main(&scope, object);

  // Create an instance of D
  Handle<Object> klass_d(&scope, findInModule(&runtime, main, "D"));
  ASSERT_TRUE(klass_d->isClass());
  Handle<Layout> layout(&scope, Class::cast(*klass_d)->instanceLayout());
  Handle<Object> instance(&scope, runtime.newInstance(layout));

  // Fetch the test function
  object = findInModule(&runtime, main, "test");
  ASSERT_TRUE(object->isFunction());
  Handle<Function> isinstance(&scope, object);

  // isinstance(1, D) should be false
  Handle<ObjectArray> args(&scope, runtime.newObjectArray(2));
  args->atPut(0, SmallInteger::fromWord(100));
  args->atPut(1, *klass_d);
  EXPECT_EQ(callFunctionToString(isinstance, args), "False\n");

  // isinstance(D, D) should be false
  args->atPut(0, *klass_d);
  args->atPut(1, *klass_d);
  EXPECT_EQ(callFunctionToString(isinstance, args), "False\n");

  // isinstance(D(), D) should be true
  args->atPut(0, *instance);
  args->atPut(1, *klass_d);
  EXPECT_EQ(callFunctionToString(isinstance, args), "True\n");

  // isinstance(D(), C) should be true
  Handle<Object> klass_c(&scope, findInModule(&runtime, main, "C"));
  ASSERT_TRUE(klass_c->isClass());
  args->atPut(1, *klass_c);
  EXPECT_EQ(callFunctionToString(isinstance, args), "True\n");

  // isinstance(D(), B) should be true
  Handle<Object> klass_b(&scope, findInModule(&runtime, main, "B"));
  ASSERT_TRUE(klass_b->isClass());
  args->atPut(1, *klass_b);
  EXPECT_EQ(callFunctionToString(isinstance, args), "True\n");

  // isinstance(C(), A) should be true
  Handle<Object> klass_a(&scope, findInModule(&runtime, main, "A"));
  ASSERT_TRUE(klass_a->isClass());
  args->atPut(1, *klass_a);
  EXPECT_EQ(callFunctionToString(isinstance, args), "True\n");
}

TEST(ThreadTest, CompareOpSmallInteger) {
  Runtime runtime;
  const char* src = R"(
a = 1
b = 2
c = 1
print(a < b)
print(a <= b)
print(a == b)
print(a >= b)
print(a > b)
print(a is c)
print(a is not c)
)";
  const char* expected = R"(True
True
False
False
False
True
False
)";
  std::string result = compileAndRunToString(&runtime, src);
  EXPECT_EQ(result, expected);
}

TEST(ThreadTest, ReplicateList) {
  const char* src = R"(
data = [1, 2, 3] * 3
for i in range(9):
  print(data[i])
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "1\n2\n3\n1\n2\n3\n1\n2\n3\n");
}

TEST(ThreadTest, InheritFromObject) {
  const char* src = R"(
class Foo(object):
  pass
)";
  Runtime runtime;
  compileAndRunToString(&runtime, src);

  // Look up the class Foo
  HandleScope scope;
  Object* object = findModule(&runtime, "__main__");
  ASSERT_TRUE(object->isModule());
  Handle<Module> main(&scope, object);
  object = findInModule(&runtime, main, "Foo");
  ASSERT_TRUE(object->isClass());
  Handle<Class> klass(&scope, object);

  // Check that its MRO is itself and object
  ASSERT_TRUE(klass->mro()->isObjectArray());
  Handle<ObjectArray> mro(&scope, klass->mro());
  ASSERT_EQ(mro->length(), 2);
  EXPECT_EQ(mro->at(0), *klass);
  EXPECT_EQ(mro->at(1), runtime.classAt(IntrinsicLayoutId::kObject));
}

// imports

TEST(ThreadTest, ImportTest) {
  Runtime runtime;
  HandleScope scope;

  const char* module_src = R"(
def say_hello():
  print("hello");
)";

  const char* main_src = R"(
import hello
hello.say_hello()
)";

  // Pre-load the hello module so is cached.
  std::unique_ptr<char[]> module_buf(Runtime::compile(module_src));
  Handle<Object> name(&scope, runtime.newStringFromCString("hello"));
  runtime.importModuleFromBuffer(module_buf.get(), name);

  std::string output = compileAndRunToString(&runtime, main_src);
  EXPECT_EQ(output, "hello\n");
}

TEST(ThreadTest, FailedImportTest) {
  Runtime runtime;
  HandleScope scope;

  const char* main_src = R"(
import hello
hello.say_hello()
)";

  EXPECT_DEATH(
      compileAndRunToString(&runtime, main_src),
      "importModule is unimplemented");
}

TEST(ThreadTest, ImportMissingAttributeTest) {
  Runtime runtime;
  HandleScope scope;

  const char* module_src = R"(
def say_hello():
  print("hello");
)";

  const char* main_src = R"(
import hello
hello.foo()
)";

  // Pre-load the hello module so is cached.
  std::unique_ptr<char[]> module_buf(Runtime::compile(module_src));
  Handle<Object> name(&scope, runtime.newStringFromCString("hello"));
  runtime.importModuleFromBuffer(module_buf.get(), name);

  EXPECT_DEATH(compileAndRunToString(&runtime, main_src), "missing attribute");
}

TEST(ThreadTest, ModuleSetAttrTest) {
  Runtime runtime;
  HandleScope scope;

  const char* module_src = R"(
def say_hello():
  print("hello");
)";

  const char* main_src = R"(
import hello
def goodbye():
  print("goodbye")
hello.say_hello = goodbye
hello.say_hello()
)";

  // Pre-load the hello module so is cached.
  std::unique_ptr<char[]> module_buf(Runtime::compile(module_src));
  Handle<Object> name(&scope, runtime.newStringFromCString("hello"));
  runtime.importModuleFromBuffer(module_buf.get(), name);

  std::string output = compileAndRunToString(&runtime, main_src);
  EXPECT_EQ(output, "goodbye\n");
}

TEST(ThreadTest, StoreFastStackEffect) {
  const char* src = R"(
def printit(x, y, z):
  print(x, y, z)

def test():
  x = 1
  y = 2
  z = 3
  printit(x, y, z)

test()
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "1 2 3\n");
}

TEST(ThreadTest, SubscriptList) {
  Runtime runtime;
  const char* src = R"(
l = [1, 2, 3, 4, 5, 6]
print(l[0], l[3], l[5])
l[0] = 6
l[5] = 1
print(l[0], l[3], l[5])
)";
  std::string output = compileAndRunToString(&runtime, src);
  ASSERT_EQ(output, "1 4 6\n6 4 1\n");
}

TEST(ThreadTest, SubscriptDict) {
  const char* src = R"(
a = {"1": 2, 2: 3}
print(a["1"])
# exceeds kInitialDictionaryCapacity
b = { 0:0, 1:1, 2:2, 3:3, 4:4, 5:5, 6:6, 7:7, 8:8, 9:9, 10:10, 11:11 }
print(b[11])
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "2\n11\n");

  const char* src1 = R"(
a = {"1": 2, 2: 3}
print(a[1])
)";
  EXPECT_DEATH(compileAndRunToString(&runtime, src1), "KeyError");
}

TEST(ThreadTest, SubscriptTuple) {
  const char* src = R"(
a = 1
b = (a, 2)
print(b[0])
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "1\n");
}

TEST(ThreadTest, BuildDictNonLiteralKey) {
  const char* src = R"(
b = "foo"
a = { b: 3, 'c': 4 }
# we need one dictionary that exceeds kInitialDictionaryCapacity
c = { b: 1, 1:1, 2:2, 3:3, 4:4, 5:5, 6:6, 7:7, 8:8, 9:9, 10:10, 11:11 }
print(a["foo"])
print(a["c"])
print(c[11])
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "3\n4\n11\n");
}

TEST(ThreadTest, PrintStackTrace) {
  const char* src = R"(
def a():
  raise 'testing 123'

def b():
  a()

def test():
  b()

test()
)";
  Runtime runtime;
  const char* re =
      "Traceback \\(most recent call last\\)\n"
      "  File '.+', line 11, in <module>\n"
      "  File '.+', line 9, in test\n"
      "  File '.+', line 6, in b\n"
      "  File '.+', line 3, in a\n";
  EXPECT_DEATH(compileAndRunToString(&runtime, src), re);
}

TEST(ThreadTest, Closure) {
  const char* src = R"(
def f():
  a = 1
  def g():
    b = 2
    def h():
      print(b)
    print(a)
    h()
    b = 3
    h()
  g()
f()
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "1\n2\n3\n");
}

TEST(ThreadTest, UnpackSequence) {
  const char* src = R"(
a, b = (1, 2)
print(a, b)
a, b = [3, 4]
print(a, b)
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "1 2\n3 4\n");
}

TEST(TheadTest, BinaryTrueDivide) {
  const char* src = R"(
a = 6
b = 2
print(a / b)
a = 5
b = 2
print(a / b)
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "3\n2.5\n");
}

TEST(ThreadTest, ListAppend) {
  const char* src = R"(
a = list()
b = list()
a.append(1)
a.append("2")
b.append(3)
a.append(b)
print(a[0], a[1], a[2][0])
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "1 2 3\n");
}

TEST(ThreadTest, ListInsert) {
  const char* src = R"(
a = list()
a.append(0)
a.append(2)
a.insert(1, 5)
print(a[0], a[1], a[2])
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "0 5 2\n");
}

TEST(ThreadTest, ListInsertExcept) {
  Runtime runtime;
  const char* src1 = R"(
a = [1, 2]
a.insert()
)";
  ASSERT_DEATH(
      compileAndRunToString(&runtime, src1),
      "aborting due to pending exception: "
      "insert\\(\\) takes exactly two arguments");

  const char* src2 = R"(
list.insert(1, 2, 3)
)";
  ASSERT_DEATH(
      compileAndRunToString(&runtime, src2),
      "aborting due to pending exception: "
      "descriptor 'insert' requires a 'list' object");

  const char* src3 = R"(
a = [1, 2]
a.insert("i", "val")
)";
  ASSERT_DEATH(
      compileAndRunToString(&runtime, src3),
      "aborting due to pending exception: "
      "index object cannot be interpreted as an integer");
}

TEST(ThreadTest, ListPop) {
  const char* src = R"(
a = [1,2,3,4,5]
a.pop()
print(len(a))
a.pop(0)
a.pop(-1)
print(len(a), a[0], a[1])
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "4\n2 2 4\n");

  const char* src2 = R"(
a = [1,2,3,4,5]
print(a.pop(), a.pop(0), a.pop(-2))
)";
  std::string output2 = compileAndRunToString(&runtime, src2);
  EXPECT_EQ(output2, "5 1 2\n");
}

TEST(ThreadTest, ListPopExcept) {
  Runtime runtime;
  const char* src1 = R"(
a = [1, 2]
a.pop(1, 2, 3, 4)
)";
  ASSERT_DEATH(
      compileAndRunToString(&runtime, src1),
      "aborting due to pending exception: "
      "pop\\(\\) takes at most 1 argument");

  const char* src2 = R"(
list.pop(1)
)";
  ASSERT_DEATH(
      compileAndRunToString(&runtime, src2),
      "aborting due to pending exception: "
      "descriptor 'pop' requires a 'list' object");

  const char* src3 = R"(
a = [1, 2]
a.pop("i")
)";
  ASSERT_DEATH(
      compileAndRunToString(&runtime, src3),
      "aborting due to pending exception: "
      "index object cannot be interpreted as an integer");

  const char* src4 = R"(
a = [1]
a.pop()
a.pop()
)";
  ASSERT_DEATH(
      compileAndRunToString(&runtime, src4),
      "unimplemented: "
      "Throw an IndexError for an out of range list");

  const char* src5 = R"(
a = [1]
a.pop(3)
)";
  ASSERT_DEATH(
      compileAndRunToString(&runtime, src5),
      "unimplemented: "
      "Throw an IndexError for an out of range list");
}

TEST(FormatTest, NoConvEmpty) {
  const char* src = R"(
print(f'')
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "\n");
}

TEST(FormatTest, NoConvOneElement) {
  const char* src = R"(
a = "hello"
x = f'a={a}'
print(x)
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "a=hello\n");
}

TEST(FormatTest, NoConvMultiElements) {
  const char* src = R"(
a = "hello"
b = "world"
c = "python"
x = f'{a} {b} {c}'
print(x)
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "hello world python\n");
}

TEST(FormatTest, NoConvMultiElementsLarge) {
  const char* src = R"(
a = "Python"
b = "is"
c = "an interpreted high-level programming language for general-purpose programming.";
x = f'{a} {b} {c}'
print(x)
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(
      output,
      "Python is an interpreted high-level programming language for general-purpose programming.\n");
}

TEST(ThreadTest, Classmethod) {
  const char* src = R"(
class Foo():
  a = 1
  @classmethod
  def bar(cls):
    print(cls.a)
a = Foo()
a.bar()
Foo.a = 2
Foo.bar()
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "1\n2\n");
}

TEST(BuildString, buildStringEmpty) {
  Runtime runtime;
  HandleScope scope;
  Handle<Code> code(&scope, runtime.newCode());

  const byte bc[] = {BUILD_STRING, 0, RETURN_VALUE, 0};
  code->setCode(runtime.newByteArrayWithAll(bc));

  Object* obj = Thread::currentThread()->run(*code);
  EXPECT_TRUE(obj->isString());
  EXPECT_TRUE(obj->isSmallString());

  Handle<String> result(&scope, obj);
  EXPECT_TRUE(result->equalsCString(""));
}

TEST(BuildString, buildStringSingle) {
  Runtime runtime;
  HandleScope scope;
  Handle<Code> code(&scope, runtime.newCode());

  Handle<ObjectArray> consts(&scope, runtime.newObjectArray(1));
  const char* expected = "foo";
  Handle<Object> str(&scope, SmallString::fromCString(expected));
  consts->atPut(0, *str);
  code->setConsts(*consts);

  const byte bc[] = {LOAD_CONST, 0, BUILD_STRING, 1, RETURN_VALUE, 0};
  code->setCode(runtime.newByteArrayWithAll(bc));

  Object* obj = Thread::currentThread()->run(*code);
  EXPECT_TRUE(obj->isString());
  EXPECT_TRUE(obj->isSmallString());

  Handle<String> result(&scope, obj);
  EXPECT_TRUE(result->equalsCString(expected));
}

TEST(BuildString, buildStringMultiSmall) {
  Runtime runtime;
  HandleScope scope;
  Handle<Code> code(&scope, runtime.newCode());

  Handle<ObjectArray> consts(&scope, runtime.newObjectArray(2));
  Handle<Object> str(&scope, SmallString::fromCString("foo"));
  Handle<Object> str1(&scope, SmallString::fromCString("bar"));
  consts->atPut(0, *str);
  consts->atPut(1, *str1);
  code->setConsts(*consts);

  const byte bc[] = {
      LOAD_CONST, 0, LOAD_CONST, 1, BUILD_STRING, 2, RETURN_VALUE, 0};
  code->setCode(runtime.newByteArrayWithAll(bc));

  Object* obj = Thread::currentThread()->run(*code);
  EXPECT_TRUE(obj->isString());
  EXPECT_TRUE(obj->isSmallString());

  Handle<String> result(&scope, obj);
  EXPECT_TRUE(result->equalsCString("foobar"));
}

TEST(BuildString, buildStringMultiLarge) {
  Runtime runtime;
  HandleScope scope;
  Handle<Code> code(&scope, runtime.newCode());

  Handle<ObjectArray> consts(&scope, runtime.newObjectArray(3));
  Handle<Object> str(&scope, SmallString::fromCString("hello"));
  Handle<Object> str1(&scope, SmallString::fromCString("world"));
  Handle<Object> str2(&scope, SmallString::fromCString("python"));
  consts->atPut(0, *str);
  consts->atPut(1, *str1);
  consts->atPut(2, *str2);
  code->setConsts(*consts);

  const byte bc[] = {LOAD_CONST,
                     0,
                     LOAD_CONST,
                     1,
                     LOAD_CONST,
                     2,
                     BUILD_STRING,
                     3,
                     RETURN_VALUE,
                     0};
  code->setCode(runtime.newByteArrayWithAll(bc));

  Object* obj = Thread::currentThread()->run(*code);
  EXPECT_TRUE(obj->isString());
  EXPECT_TRUE(obj->isLargeString());

  Handle<String> result(&scope, obj);
  EXPECT_TRUE(result->equalsCString("helloworldpython"));
}

TEST(UnpackSeq, unpackRangePyStone) {
  const char* src = R"(
[Ident1, Ident2, Ident3, Ident4, Ident5] = range(1, 6)
print(Ident1, Ident2, Ident3, Ident4, Ident5)
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "1 2 3 4 5\n");
}

TEST(UnpackSeq, unpackRange) {
  const char* src = R"(
[a ,b, c] = range(2, 5)
print(a, b, c)
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "2 3 4\n");
}

// LIST_APPEND(listAdd) in list_comp, followed by unpack
// TODO(rkng): list support in BINARY_ADD
TEST(UnpackList, unpackListCompAppend) {
  const char* src = R"(
a = [1, 2, 3]
b = [x for x in a]
b1, b2, b3 = b
print(b1, b2, b3)
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "1 2 3\n");
}

TEST(UnpackList, unpackNestedLists) {
  const char* src = R"(
b = [[1,2], [3,4,5]]
b1, b2 = b
b11, b12 = b1
b21, b22, b23 = b2
print(len(b), len(b1), len(b2), b11, b12, b21, b22, b23)
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "2 2 3 1 2 3 4 5\n");
}

TEST(UnpackSeq, unpackRangeStep) {
  const char* src = R"(
[a ,b, c, d] = range(2, 10, 2)
print(a, b, c, d)
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "2 4 6 8\n");
}

TEST(UnpackSeq, unpackRangeNeg) {
  const char* src = R"(
[a ,b, c, d, e] = range(-10, 0, 2)
print(a, b, c, d, e)
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "-10 -8 -6 -4 -2\n");
}

TEST(ListIterTest, build) {
  const char* src = R"(
a = [1, 2, 3]
for x in a:
  print(x)
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "1\n2\n3\n");
}

TEST(ListAppendTest, buildAndUnpack) {
  const char* src = R"(
a = [1, 2]
b = [x for x in [a] * 3]
b1, b2, b3 = b
b11, b12 = b1
print(len(b), len(b1), b11, b12)
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "3 2 1 2\n");
}

TEST(ThreadTest, SubclassList) {
  const char* src = R"(
class Foo():
  def __init__(self):
    self.a = "a"
class Bar(Foo, list): pass
a = Bar()
a.append(1)
print(a[0], a.a)
a.insert(0, 2)
print(a[0], a[1])
a.pop()
print(a[0])
a.remove(2)
print(len(a))
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "1 a\n2 1\n2\n0\n");
}

TEST(ThreadTest, BaseClassConflict) {
  const char* src = R"(
class Foo(list, dict): pass
)";
  Runtime runtime;
  EXPECT_DEATH(compileAndRunToString(&runtime, src), "lay-out conflict");
}

TEST(BuildSlice, noneSliceCopyList) {
  const char* src = R"(
a = [1, 2, 3]
b = a[:]
print(len(b), b[0], b[1], b[2])
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "3 1 2 3\n");
}

TEST(BuildSlice, sliceOperations) {
  const char* src = R"(
a = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
b = a[1:2:3]
print(len(b), b[0])
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "1 2\n");

  const char* src2 = R"(
a = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
b = a[1::3]
print(len(b), b[0], b[1], b[2])
)";
  std::string output2 = compileAndRunToString(&runtime, src2);
  EXPECT_EQ(output2, "3 2 5 8\n");

  const char* src3 = R"(
a = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
b = a[8:2:-2]
print(len(b), b[0], b[1], b[2])
)";
  std::string output3 = compileAndRunToString(&runtime, src3);
  ASSERT_EQ(output3, "3 9 7 5\n");

  const char* src4 = R"(
a = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
b = a[8:2:2]
print(len(b))
)";
  std::string output4 = compileAndRunToString(&runtime, src4);
  EXPECT_EQ(output4, "0\n");
}

TEST(BuildSlice, noneSliceCopyListComp) { // pystone
  const char* src = R"(
a = [1, 2, 3]
b = [x[:] for x in [a] * 2]
c = b is a
b1, b2 = b
b11, b12, b13 = b1
print(c, len(b), len(b1), b11, b12, b13)
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "False 2 3 1 2 3\n");
}

TEST(BuildSlice, pystone) { // pystone src
  const char* src = R"(
Array1Glob = [0]*51
Array2Glob = [x[:] for x in [Array1Glob]*51]
print(len(Array1Glob), len(Array2Glob), len(Array2Glob[0]))
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "51 51 51\n");
}

TEST(ThreadTest, BreakLoopWhileLoop) {
  const char* src = R"(
a = 0
while 1:
    a = a + 1
    print(a)
    if a == 3:
        break
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "1\n2\n3\n");
}

TEST(ThreadTest, BreakLoopWhileLoop1) {
  const char* src = R"(
a = 0
while 1:
    a = a + 1
    print(a)
    if a == 3:
        break
print("ok",a)
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "1\n2\n3\nok 3\n");
}

TEST(ThreadTest, BreakLoopWhileLoopBytecode) {
  Runtime runtime;
  HandleScope scope;

  Handle<ObjectArray> consts(&scope, runtime.newObjectArray(4));
  Handle<Code> code(&scope, runtime.newCode());
  consts->atPut(0, SmallInteger::fromWord(0));
  consts->atPut(1, SmallInteger::fromWord(1));
  consts->atPut(2, SmallInteger::fromWord(3));
  consts->atPut(3, None::object());
  code->setConsts(*consts);

  Handle<ObjectArray> names(&scope, runtime.newObjectArray(1));
  Handle<Object> key(&scope, runtime.newStringFromCString("a"));
  names->atPut(0, *key);
  code->setNames(*names);

  // see python code in BreakLoop.whileLoop (sans print)
  const byte bc[] = {LOAD_CONST,        0, // 0
                     STORE_NAME,        0, // a
                     SETUP_LOOP,        22, LOAD_NAME,  0, // a
                     LOAD_CONST,        1, // 1
                     BINARY_ADD,        0,  STORE_NAME, 0, // a
                     LOAD_NAME,         0, // a
                     LOAD_CONST,        2, // 3
                     COMPARE_OP,        2, // ==
                     POP_JUMP_IF_FALSE, 6,  BREAK_LOOP, 0, JUMP_ABSOLUTE, 6,
                     POP_BLOCK,         0,  LOAD_CONST, 3, // None
                     RETURN_VALUE,      0};
  code->setCode(runtime.newByteArrayWithAll(bc));

  Thread* thread = Thread::currentThread();
  Frame* frame = thread->pushFrame(*code);

  Handle<Dictionary> implicit_globals(&scope, runtime.newDictionary());
  Handle<Dictionary> builtins(&scope, runtime.newDictionary());

  frame->setImplicitGlobals(*implicit_globals);
  frame->setFastGlobals(
      runtime.computeFastGlobals(code, implicit_globals, builtins));

  Handle<Object> result(&scope, Interpreter::execute(thread, frame));
  Handle<Object> value(&scope, runtime.dictionaryAt(implicit_globals, key));
  ASSERT_TRUE(value->isValueCell());
  Object* valueObj = ValueCell::cast(*value)->value();
  EXPECT_TRUE(valueObj->isSmallInteger());
  EXPECT_EQ(SmallInteger::cast(valueObj)->value(), 3);
}

TEST(ThreadTest, BreakLoopRangeLoop) {
  const char* src = R"(
for x in range(1,6):
  if x == 3:
    break;
  print(x)
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "1\n2\n");
}

TEST(ThreadTest, Func2TestPyStone) { // mimic pystone.py Func2
  const char* src = R"(
def f1(x, y):
  return x + y
def f2():
  return f1(1, 2)
print(f2())
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "3\n");
}

TEST(ThreadTest, TruthyIntPos) {
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

TEST(ThreadTest, TruthyIntNeg) {
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

TEST(ThreadTest, RichCompareStringEQ) { // pystone dependency
  const char* src = R"(
a = "__main__"
if (a == "__main__"):
  print("foo")
else:
  print("bar")
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "foo\n");
}

TEST(ThreadTest, RichCompareStringNE) { // pystone dependency
  const char* src = R"(
a = "__main__"
if (a != "__main__"):
  print("foo")
else:
  print("bar")
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "bar\n");
}

TEST(ThreadTest, RichCompareSingleCharLE) { // pystone dependency
  const char* src = R"(
a = ['h','e','l','l','o']
for x in a:
  if x <= 'i':
    print("L")
  else:
    print("x")
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "L\nL\nx\nx\nx\n");
}

TEST(ThreadTest, BinSubscrString) { // pystone dependency
  const char* src = R"(
a = 'Hello'
print(a[0],a[1],a[2],a[3],a[4])
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "H e l l o\n");
}

TEST(ThreadTest, SuperTest1) {
  const char* src = R"(
class A:
    def f(self):
        return 1

class B(A):
    def f(self):
        return super(B, self).f() + 2

class C(A):
    def f(self):
        return super(C, self).f() + 3

class D(C, B):
    def f(self):
        return super(D, self).f() + 4

class E(D):
    pass

class F(E):
    f = E.f

class G(A):
    pass

print(D().f())
print(D.f(D()))
print(E().f())
print(E.f(E()))
print(F().f())
print(F.f(F()))
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "10\n10\n10\n10\n10\n10\n");
}

TEST(ThreadTest, SuperTest2) {
  const char* src = R"(
class A:
    @classmethod
    def cm(cls):
        return (cls, 1)

class B(A):
    @classmethod
    def cm(cls):
        return (cls, super(B, cls).cm(), 2)

class C(A):
    @classmethod
    def cm(cls):
        return (cls, super(C, cls).cm(), 3)

class D(C, B):
    def cm(cls):
        return (cls, super(D, cls).cm(), 4)

class E(D):
    pass

class G(A):
    pass

print(A.cm() == (A, 1))
print(A().cm() == (A, 1))
print(G.cm() == (G, 1))
print(G().cm() == (G, 1))
d = D()
print(d.cm() == (d, (D, (D, (D, 1), 2), 3), 4))
e = E()
print(e.cm() == (e, (E, (E, (E, 1), 2), 3), 4))
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "True\nTrue\nTrue\nTrue\nTrue\nTrue\n");
}

TEST(ThreadTest, ListRemove) {
  const char* src = R"(
a = [5, 4, 3, 2, 1]
a.remove(2)
a.remove(5)
print(len(a), a[0], a[1], a[2])
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "3 4 3 1\n");
}

TEST(ThreadTest, SysArgvProgArg) { // pystone dependency
  const char* src = R"(
import sys
print(len(sys.argv))

for x in sys.argv:
  print(x)
)";
  Runtime runtime;
  const char* argv[2];
  argv[0] = "./python"; // program
  argv[1] = "SysArgv"; // script
  runtime.setArgv(2, argv);
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "1\nSysArgv\n");
}

TEST(ThreadTest, SysArgvMultiArgs) { // pystone dependency
  const char* src = R"(
import sys
print(len(sys.argv))

print(sys.argv[1])

for x in sys.argv:
  print(x)
)";
  Runtime runtime;
  const char* argv[3];
  argv[0] = "./python"; // program
  argv[1] = "SysArgv"; // script
  argv[2] = "200"; // argument
  runtime.setArgv(3, argv);
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "2\n200\nSysArgv\n200\n");
}

TEST(ThreadTest, SetupExceptNoOp) { // pystone dependency
  const char* src = R"(
def f(x):
  try: print(x)
  except ValueError:
    print("Invalid Argument")
f(100)
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "100\n");
}

TEST(ThreadTest, SysExit) {
  const char* src = R"(
import sys
sys.exit()
)";
  Runtime runtime;
  ASSERT_EXIT(
      compileAndRunToString(&runtime, src), ::testing::ExitedWithCode(0), "");
}

TEST(ThreadTest, SysExitCode) { // pystone dependency
  const char* src = R"(
import sys
sys.exit(100)
)";
  Runtime runtime;
  ASSERT_EXIT(
      compileAndRunToString(&runtime, src), ::testing::ExitedWithCode(100), "");
}

TEST(ThreadTest, BuiltinInt) { // pystone dependency
  const char* src = R"(
a = int("123")
b = int("-987")
print(a == 123, b == -987, a > b, a, b)
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "True True True 123 -987\n");
}

TEST(ThreadTest, TimeTime) { // pystone dependency
  const char* src = R"(
import time
t = time.time()
print(t.__class__ is float)
)";

  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "True\n");
}

TEST(ThreadTest, TimeTimeComp) { // pystone dependency
  const char* src = R"(
import time
t = time.time()
for i in range(3):
  print(i)
t1 = time.time()
print(t1 > t, t > t1, t == t1)
)";

  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "0\n1\n2\nTrue False False\n");
}

TEST(ThreadTest, TimeTimeFromImport) { // pystone dependency
  const char* src = R"(
from time import time
t = time()
print(t.__class__ is float)
)";

  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "True\n");
}

TEST(ThreadTest, ImportFromNeg) {
  const char* src = R"(
from time import foobarbaz
)";

  Runtime runtime;
  EXPECT_DEATH(compileAndRunToString(&runtime, src);, "cannot import name\n");
}

TEST(ThreadTest, SysStdOutErr) { // pystone dependency
  const char* src = R"(
import sys
print(sys.stdout, sys.stderr)
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "1 2\n");
}

TEST(ThreadTest, BuiltInPrintStdOut) {
  const char* src = R"(
import sys
print("hello", file=sys.stdout)
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "hello\n");
}

TEST(ThreadTest, BuiltInPrintEnd) {
  const char* src = R"(
import sys
print("hi", end='ho')
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "hiho");
}

TEST(ThreadTest, BuiltInPrintStdOutEnd) {
  const char* src = R"(
import sys
print("hi", end='ho', file=sys.stdout)
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "hiho");
}

TEST(ThreadTest, BuiltInPrintStdErr) { // pystone dependency
  const char* src = R"(
import sys
print("hi", file=sys.stderr, end='ya')
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "hiya");
}

} // namespace python

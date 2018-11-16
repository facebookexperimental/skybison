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

TEST(ThreadTest, OverlappingFrames) {
  Runtime runtime;
  HandleScope scope;

  // Push a frame for a code object with space for 3 items on the value stack
  Handle<Code> callerCode(&scope, runtime.newCode());
  callerCode->setStacksize(3);
  auto thread = Thread::currentThread();
  auto callerFrame = thread->pushFrame(*callerCode, thread->initialFrame());
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
  code->setNlocals(6);
  auto frame = thread->pushFrame(*code, callerFrame);

  // Make sure we can read the args from the frame
  Object** locals = frame->locals();

  ASSERT_TRUE(locals[0]->isSmallInteger());
  EXPECT_EQ(SmallInteger::cast(locals[3])->value(), arg3->value());

  ASSERT_TRUE(locals[1]->isSmallInteger());
  EXPECT_EQ(SmallInteger::cast(locals[4])->value(), arg2->value());

  ASSERT_TRUE(locals[2]->isSmallInteger());
  EXPECT_EQ(SmallInteger::cast(locals[5])->value(), arg1->value());
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
  byte* prevSp = thread->ptr();
  auto frame = thread->pushFrame(*code, thread->initialFrame());

  // Verify frame invariants post-push
  EXPECT_EQ(frame->previousFrame(), thread->initialFrame());
  EXPECT_EQ(frame->code(), *code);
  EXPECT_EQ(frame->valueStackTop(), reinterpret_cast<Object**>(frame));
  EXPECT_EQ(frame->valueStackBase(), frame->valueStackTop());
  EXPECT_EQ(
      frame->locals() + code->nlocals(), reinterpret_cast<Object**>(prevSp));
  EXPECT_EQ(frame->previousSp(), prevSp);

  // Make sure we restore the thread's stack pointer back to its previous
  // location
  thread->popFrame(frame);
  EXPECT_EQ(thread->ptr(), prevSp);
}

TEST(ThreadTest, PushFrameWithNoCellVars) {
  Runtime runtime;
  HandleScope scope;

  Handle<Code> code(&scope, runtime.newCode());
  code->setCellvars(None::object());
  code->setFreevars(runtime.newObjectArray(0));
  auto thread = Thread::currentThread();
  byte* prevSp = thread->ptr();
  thread->pushFrame(*code, thread->initialFrame());

  EXPECT_EQ(thread->ptr(), prevSp - Frame::kSize);
}

TEST(ThreadTest, PushFrameWithNoFreeVars) {
  Runtime runtime;
  HandleScope scope;

  Handle<Code> code(&scope, runtime.newCode());
  code->setFreevars(None::object());
  code->setCellvars(runtime.newObjectArray(0));
  auto thread = Thread::currentThread();
  byte* prevSp = thread->ptr();
  thread->pushFrame(*code, thread->initialFrame());

  EXPECT_EQ(thread->ptr(), prevSp - Frame::kSize);
}

TEST(ThreadTest, ManipulateValueStack) {
  Runtime runtime;
  HandleScope scope;
  auto thread = Thread::currentThread();
  auto frame = thread->openAndLinkFrame(0, 3, thread->initialFrame());

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
  auto frame = thread->openAndLinkFrame(0, 0, thread->initialFrame());
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
  Frame* frame = thread->pushFrame(*code, thread->initialFrame());

  Handle<Dictionary> globals(&scope, runtime.newDictionary());
  Handle<ValueCell> value_cell(&scope, runtime.newValueCell());
  value_cell->setValue(SmallInteger::fromWord(1234));
  Handle<Object> value(&scope, *value_cell);
  runtime.dictionaryAtPut(globals, key, value);
  frame->setGlobals(*globals);

  Handle<Object> result(&scope, Interpreter::execute(thread, frame));
  EXPECT_EQ(*result, value_cell->value());
}

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
  Frame* frame = thread->pushFrame(*code, thread->initialFrame());

  Handle<Dictionary> globals(&scope, runtime.newDictionary());
  frame->setGlobals(*globals);

  Handle<Object> result(&scope, Interpreter::execute(thread, frame));

  Handle<Object> value(&scope, None::object());
  bool is_present = runtime.dictionaryAt(globals, key, value.pointer());
  ASSERT_TRUE(is_present);
  Handle<ValueCell> value_cell(&scope, *value);
  EXPECT_EQ(*result, value_cell->value());
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
  Frame* frame = thread->pushFrame(*code, thread->initialFrame());

  Handle<ValueCell> value_cell1(&scope, runtime.newValueCell());
  value_cell1->setValue(SmallInteger::fromWord(99));

  Handle<Dictionary> globals(&scope, runtime.newDictionary());
  Handle<Object> value(&scope, *value_cell1);
  runtime.dictionaryAtPut(globals, key, value);
  frame->setGlobals(*globals);

  Handle<Object> result(&scope, Interpreter::execute(thread, frame));

  Handle<Object> value_cell2(&scope, None::object());
  bool is_present = runtime.dictionaryAt(globals, key, value_cell2.pointer());
  ASSERT_TRUE(is_present);
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
  Frame* frame = thread->pushFrame(*code, thread->initialFrame());

  Handle<Dictionary> implicit_globals(&scope, runtime.newDictionary());
  frame->setImplicitGlobals(*implicit_globals);

  Handle<Object> result(&scope, Interpreter::execute(thread, frame));

  Handle<Object> value(&scope, None::object());
  bool is_present =
      runtime.dictionaryAt(implicit_globals, key, value.pointer());
  ASSERT_TRUE(is_present);
  Handle<ValueCell> value_cell(&scope, *value);
  EXPECT_EQ(*result, value_cell->value());
}

TEST(ThreadTest, MakeFunction) {
  Runtime runtime;
  HandleScope scope;

  Handle<Code> module(&scope, runtime.newCode());

  Handle<ObjectArray> consts(&scope, runtime.newObjectArray(3));
  consts->atPut(0, runtime.newCode());
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

  Thread* thread = Thread::currentThread();
  Frame* frame = thread->pushFrame(*module, thread->initialFrame());

  Handle<Dictionary> implicit_globals(&scope, runtime.newDictionary());
  frame->setImplicitGlobals(*implicit_globals);

  Handle<Object> result(&scope, Interpreter::execute(thread, frame));

  Handle<Object> value(&scope, None::object());
  bool is_present =
      runtime.dictionaryAt(implicit_globals, key, value.pointer());
  ASSERT_TRUE(is_present);
  Handle<ValueCell> value_cell(&scope, *value);
  ASSERT_TRUE(value_cell->value()->isFunction());

  Handle<Function> function(&scope, value_cell->value());
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

  ASSERT_TRUE(list->at(1)->isString());
  EXPECT_TRUE(
      String::cast(list->at(1))->equals(runtime.newStringFromCString("qqq")));

  EXPECT_EQ(list->at(2), None::object());
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
  auto frame = thread->pushFrame(*code, thread->initialFrame());
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
  auto frame = thread->pushFrame(*code, thread->initialFrame());
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

TEST(ThreadTest, LoadBuildClassEmptyClass) {
  Runtime runtime;
  HandleScope scope;

  // class C:
  //   pass
  const char buffer[] =
      "\x33\x0D\x0D\x0A\x42\x98\x29\x5A\x12\x00\x00\x00\xE3\x00\x00\x00\x00\x00"
      "\x00\x00\x00\x00\x00\x00\x00\x03\x00\x00\x00\x40\x00\x00\x00\x73\x12\x00"
      "\x00\x00\x47\x00\x64\x00\x64\x01\x84\x00\x64\x01\x83\x02\x5A\x00\x64\x02"
      "\x53\x00\x29\x03\x63\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x01"
      "\x00\x00\x00\x40\x00\x00\x00\x73\x0C\x00\x00\x00\x65\x00\x5A\x01\x64\x00"
      "\x5A\x02\x64\x01\x53\x00\x29\x02\xDA\x01\x43\x4E\x29\x03\xDA\x08\x5F\x5F"
      "\x6E\x61\x6D\x65\x5F\x5F\xDA\x0A\x5F\x5F\x6D\x6F\x64\x75\x6C\x65\x5F\x5F"
      "\xDA\x0C\x5F\x5F\x71\x75\x61\x6C\x6E\x61\x6D\x65\x5F\x5F\xA9\x00\x72\x05"
      "\x00\x00\x00\x72\x05\x00\x00\x00\xFA\x0F\x64\x65\x66\x69\x6E\x65\x61\x63"
      "\x6C\x61\x73\x73\x2E\x70\x79\x72\x01\x00\x00\x00\x01\x00\x00\x00\x73\x02"
      "\x00\x00\x00\x08\x01\x72\x01\x00\x00\x00\x4E\x29\x01\x72\x01\x00\x00\x00"
      "\x72\x05\x00\x00\x00\x72\x05\x00\x00\x00\x72\x05\x00\x00\x00\x72\x06\x00"
      "\x00\x00\xDA\x08\x3C\x6D\x6F\x64\x75\x6C\x65\x3E\x01\x00\x00\x00\x73\x00"
      "\x00\x00\x00";

  Object* result = runtime.run(buffer);
  ASSERT_EQ(result, None::object()); // returns None

  Handle<Module> mod(&scope, runtime.findModule("__main__"));
  ASSERT_TRUE(mod->isModule());

  Handle<Dictionary> dict(&scope, mod->dictionary());
  ASSERT_TRUE(dict->isDictionary());

  Handle<Object> key(&scope, runtime.newStringFromCString("C"));
  Handle<Object> value(&scope, None::object());
  bool found = runtime.dictionaryAt(dict, key, value.pointer());
  ASSERT_TRUE(found);
  ASSERT_TRUE(value->isValueCell());

  Handle<Class> cls(&scope, ValueCell::cast(*value)->value());
  ASSERT_TRUE(cls->name()->isString());
  EXPECT_TRUE(String::cast(cls->name())->equalsCString("C"));

  Handle<ObjectArray> mro(&scope, cls->mro());
  EXPECT_EQ(mro->length(), 2);
  EXPECT_EQ(mro->at(0), *cls);
  EXPECT_EQ(mro->at(1), runtime.classAt(ClassId::kObject));
}

TEST(ThreadTest, LoadBuildClassClassWithInit) {
  Runtime runtime;
  HandleScope scope;

  // class C:
  //   def __init__(self):
  //     pass

  const char buffer[] =
      "\x33\x0D\x0D\x0A\xB6\xF8\x2E\x5A\x2F\x00\x00\x00\xE3\x00\x00\x00\x00\x00"
      "\x00\x00\x00\x00\x00\x00\x00\x03\x00\x00\x00\x40\x00\x00\x00\x73\x12\x00"
      "\x00\x00\x47\x00\x64\x00\x64\x01\x84\x00\x64\x01\x83\x02\x5A\x00\x64\x02"
      "\x53\x00\x29\x03\x63\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x02"
      "\x00\x00\x00\x40\x00\x00\x00\x73\x14\x00\x00\x00\x65\x00\x5A\x01\x64\x00"
      "\x5A\x02\x64\x01\x64\x02\x84\x00\x5A\x03\x64\x03\x53\x00\x29\x04\xDA\x01"
      "\x43\x63\x01\x00\x00\x00\x00\x00\x00\x00\x01\x00\x00\x00\x01\x00\x00\x00"
      "\x43\x00\x00\x00\x73\x04\x00\x00\x00\x64\x00\x53\x00\x29\x01\x4E\xA9\x00"
      "\x29\x01\xDA\x04\x73\x65\x6C\x66\x72\x02\x00\x00\x00\x72\x02\x00\x00\x00"
      "\xFA\x10\x64\x65\x66\x69\x6E\x65\x61\x63\x6C\x61\x73\x73\x32\x2E\x70\x79"
      "\xDA\x08\x5F\x5F\x69\x6E\x69\x74\x5F\x5F\x02\x00\x00\x00\x73\x02\x00\x00"
      "\x00\x00\x01\x7A\x0A\x43\x2E\x5F\x5F\x69\x6E\x69\x74\x5F\x5F\x4E\x29\x04"
      "\xDA\x08\x5F\x5F\x6E\x61\x6D\x65\x5F\x5F\xDA\x0A\x5F\x5F\x6D\x6F\x64\x75"
      "\x6C\x65\x5F\x5F\xDA\x0C\x5F\x5F\x71\x75\x61\x6C\x6E\x61\x6D\x65\x5F\x5F"
      "\x72\x05\x00\x00\x00\x72\x02\x00\x00\x00\x72\x02\x00\x00\x00\x72\x02\x00"
      "\x00\x00\x72\x04\x00\x00\x00\x72\x01\x00\x00\x00\x01\x00\x00\x00\x73\x02"
      "\x00\x00\x00\x08\x01\x72\x01\x00\x00\x00\x4E\x29\x01\x72\x01\x00\x00\x00"
      "\x72\x02\x00\x00\x00\x72\x02\x00\x00\x00\x72\x02\x00\x00\x00\x72\x04\x00"
      "\x00\x00\xDA\x08\x3C\x6D\x6F\x64\x75\x6C\x65\x3E\x01\x00\x00\x00\x73\x00"
      "\x00\x00\x00";

  Object* result = runtime.run(buffer);
  ASSERT_EQ(result, None::object()); // returns None

  Handle<Module> mod(&scope, runtime.findModule("__main__"));
  ASSERT_TRUE(mod->isModule());

  Handle<Dictionary> mod_dict(&scope, mod->dictionary());
  ASSERT_TRUE(mod_dict->isDictionary());

  Handle<Object> value(&scope, None::object());

  // Check for the class name in the module dictionary
  Handle<Object> cls_name(&scope, runtime.newStringFromCString("C"));
  ASSERT_TRUE(runtime.dictionaryIncludes(mod_dict, cls_name));

  runtime.dictionaryAt(mod_dict, cls_name, value.pointer());
  Handle<Class> cls(&scope, ValueCell::cast(*value)->value());

  // Check class MRO
  Handle<ObjectArray> mro(&scope, cls->mro());
  EXPECT_EQ(mro->length(), 2);
  EXPECT_EQ(mro->at(0), *cls);
  EXPECT_EQ(mro->at(1), runtime.classAt(ClassId::kObject));

  // Check class name
  ASSERT_TRUE(cls->name()->isString());
  EXPECT_TRUE(String::cast(cls->name())->equalsCString("C"));

  Handle<Dictionary> cls_dict(&scope, cls->dictionary());
  ASSERT_TRUE(cls_dict->isDictionary());

  // Check for the __init__ method name in the dictionary
  Handle<Object> meth_name(&scope, runtime.newStringFromCString("__init__"));
  ASSERT_TRUE(runtime.dictionaryIncludes(cls_dict, meth_name));
  runtime.dictionaryAt(cls_dict, meth_name, value.pointer());
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

} // namespace python

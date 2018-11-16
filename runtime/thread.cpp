#include "thread.h"

#include <cstring>

#include "frame.h"
#include "globals.h"
#include "handles.h"
#include "interpreter.h"
#include "objects.h"

namespace python {

thread_local Thread* current_thread_ = nullptr;

Thread::Thread(word size)
    : handles_(new Handles()),
      size_(Utils::roundUp(size, kPointerSize)),
      next_(nullptr),
      runtime_(nullptr) {
  start_ = new byte[size];
  // Stack growns down in order to match machine convention
  end_ = ptr_ = start_ + size;
  pushInitialFrame();
}

Thread::~Thread() {
  delete handles_;
  delete[] start_;
}

Thread* Thread::currentThread() {
  return current_thread_;
}

void Thread::setCurrentThread(Thread* thread) {
  current_thread_ = thread;
}

Frame* Thread::openAndLinkFrame(
    word localsSize,
    word stackSize,
    Frame* previousFrame) {
  word size = Frame::kSize + (localsSize + stackSize) * kPointerSize;

  // allocate that much space on the stack
  // TODO: Grow stack
  byte* prevSp = ptr_;
  assert(ptr_ - size >= start_);
  ptr_ -= size;

  // Take care to align the frame such that the arguments that were pushed on
  // the stack by the caller are adjacent to the locals of the callee.
  byte* sp = ptr_;
  if (!previousFrame->isSentinelFrame()) {
    sp = reinterpret_cast<byte*>(previousFrame->valueStackTop()) - size;
  }
  auto frame = reinterpret_cast<Frame*>(sp + stackSize * kPointerSize);

  // Initialize the frame.
  std::memset(ptr_, 0, size);
  frame->setPreviousFrame(previousFrame);
  frame->setValueStackTop(reinterpret_cast<Object**>(frame));
  frame->setPreviousSp(prevSp);

  // return a pointer to the base of the frame
  return frame;
}

Frame* Thread::pushFrame(Object* object, Frame* previousFrame) {
  Code* code = Code::cast(object);
  word ncells = ObjectArray::cast(code->cellvars())->length();
  word nfrees = ObjectArray::cast(code->freevars())->length();
  word nlocals = code->nlocals() - code->argcount() + ncells + nfrees;
  auto frame = openAndLinkFrame(nlocals, code->stacksize(), previousFrame);
  frame->setCode(code);
  return frame;
}

Frame* Thread::pushModuleFunctionFrame(
    Module* module,
    Object* object,
    Frame* previousFrame) {
  Frame* result = pushFrame(object, previousFrame);
  result->setGlobals(module->dictionary());
  result->setImplicitGlobals(module->dictionary());
  return result;
}

Frame* Thread::pushClassFunctionFrame(
    Object* function,
    Object* dictionary,
    Frame* caller) {
  Object* code = Function::cast(function)->code();
  Frame* result = pushFrame(code, caller);
  result->setGlobals(Function::cast(function)->globals());
  result->setImplicitGlobals(dictionary);
  return result;
}

void Thread::pushInitialFrame() {
  assert(ptr_ == end_);
  assert(ptr_ - Frame::kSize > start_);

  ptr_ -= Frame::kSize;
  initialFrame_ = reinterpret_cast<Frame*>(ptr_);
  initialFrame_->makeSentinel();
}

void Thread::popFrame(Frame* frame) {
  assert(!frame->isSentinelFrame());
  ptr_ = frame->previousSp();
}

Object* Thread::run(Object* object) {
  assert(ptr_ == reinterpret_cast<byte*>(initialFrame_));
  Frame* frame = pushFrame(object, initialFrame_);
  return Interpreter::execute(this, frame);
}

Object* Thread::runModuleFunction(Module* module, Object* object) {
  assert(ptr_ == reinterpret_cast<byte*>(initialFrame_));
  Frame* frame = pushModuleFunctionFrame(module, object, initialFrame_);
  return Interpreter::execute(this, frame);
}

Object*
Thread::runClassFunction(Object* function, Object* dictionary, Frame* caller) {
  Frame* frame = pushClassFunctionFrame(function, dictionary, caller);
  return Interpreter::execute(this, frame);
}

} // namespace python

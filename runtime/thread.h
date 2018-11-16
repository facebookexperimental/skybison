#pragma once

#include "globals.h"

namespace python {

class Class;
class Dictionary;
class Frame;
class Function;
class Handles;
class Module;
class Object;
class ObjectArray;
class PointerVisitor;
class Runtime;
class String;

class Thread {
 public:
  static const int kDefaultStackSize = 1 * MiB;

  explicit Thread(word size);
  ~Thread();

  static Thread* currentThread();
  static void setCurrentThread(Thread* thread);

  byte* ptr() {
    return ptr_;
  }

  Frame* openAndLinkFrame(
      word numArgs,
      word numVars,
      word stackDepth,
      Frame* previousFrame);
  Frame* pushFrame(Object* code, Frame* previousFrame);
  Frame*
  pushModuleFunctionFrame(Module* module, Object* code, Frame* previousFrame);
  Frame*
  pushClassFunctionFrame(Object* function, Object* dictionary, Frame* caller);

  void popFrame(Frame* frame);

  Object* run(Object* object);
  Object* runModuleFunction(Module* module, Object* object);
  Object* runClassFunction(Object* function, Object* dictionary, Frame* caller);

  Thread* next() {
    return next_;
  }

  Handles* handles() {
    return handles_;
  }

  Runtime* runtime() {
    return runtime_;
  }

  Frame* initialFrame() {
    return initialFrame_;
  }

  void visitRoots(PointerVisitor* visitor);

  void setRuntime(Runtime* runtime) {
    runtime_ = runtime;
  }

  // Exception API
  //
  // Native code that wishes to throw an exception into python must do two
  // things:
  //
  // 1. Call `throwException` or one of its convenience wrappers.
  // 2. Return an Error object from the native entry point.
  //
  // It is an error to do one of these but not the other. When the native entry
  // point returns, the pending exception will be raised in the python
  // interpreter.
  //
  // Note that it is perfectly ok to use the Error return value internally
  // without throwing exceptions. The restriction on returning an Error only
  // applies to native entry points.

  // Instantiates an exception with the given arguments and posts it to be
  // thrown upon returning to managed code.
  // TODO: decide on the signature for this function.
  // void throwException(...);

  // Convenience method for throwing a RuntimeError exception with an error
  // message.
  void throwRuntimeError(String* message);
  Object* throwRuntimeErrorFromCString(const char* message);

  // Convenience method for throwing a TypeError exception with an error
  // message.
  void throwTypeError(String* message);
  Object* throwTypeErrorFromCString(const char* message);

  // Convenience method for throwing a ValueError exception with an error
  // message.
  void throwValueError(String* message);
  void throwValueErrorFromCString(const char* message);

  // Gets the pending exception object - if it is None, no exception has been
  // posted.
  Object* pendingException();

 private:
  void pushInitialFrame();

  Handles* handles_;

  word size_;
  byte* start_;
  byte* end_;
  byte* ptr_;

  Frame* initialFrame_;
  Thread* next_;
  Runtime* runtime_;

  // A pending exception object which should be thrown upon returning to
  // managed code. Is set to None if there is no pending exception.
  Object* pending_exception_;

  DISALLOW_COPY_AND_ASSIGN(Thread);
};

} // namespace python

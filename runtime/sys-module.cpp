#include "sys-module.h"
#include <unistd.h>

#include <cerrno>
#include <cstdio>
#include <cstring>

#include "builtins-module.h"
#include "bytes-builtins.h"
#include "exception-builtins.h"
#include "frame.h"
#include "frozen-modules.h"
#include "globals.h"
#include "handles.h"
#include "int-builtins.h"
#include "module-builtins.h"
#include "objects.h"
#include "os.h"
#include "runtime.h"
#include "thread.h"

namespace python {

const char* const SysModule::kFrozenData = kSysModuleData;

static void writeImpl(Thread* thread, const Object& file, FILE* fallback_fp,
                      const char* format, va_list va) {
  HandleScope scope(thread);
  Object type(&scope, thread->pendingExceptionType());
  Object value(&scope, thread->pendingExceptionValue());
  Object tb(&scope, thread->pendingExceptionTraceback());
  thread->clearPendingException();

  static const char truncated[] = "... truncated";
  static constexpr int message_size = 1001;
  char buffer[message_size + sizeof(truncated) - 1];
  int written = std::vsnprintf(buffer, message_size, format, va);
  CHECK(written >= 0, "vsnprintf failed");
  if (written >= message_size) {
    std::memcpy(&buffer[message_size - 1], truncated, sizeof(truncated));
    written = message_size + sizeof(truncated) - 2;
  }
  Str str(&scope, thread->runtime()->newStrWithAll(
                      View<byte>(reinterpret_cast<byte*>(buffer), written)));
  if (file.isNoneType() ||
      thread->invokeMethod2(file, SymbolId::kWrite, str).isError()) {
    fwrite(buffer, 1, written, fallback_fp);
  }

  thread->clearPendingException();
  thread->setPendingExceptionType(*type);
  thread->setPendingExceptionValue(*value);
  thread->setPendingExceptionTraceback(*tb);
}

void writeStdout(Thread* thread, const char* format, ...) {
  va_list va;
  va_start(va, format);
  writeStdoutV(thread, format, va);
  va_end(va);
}

void writeStdoutV(Thread* thread, const char* format, va_list va) {
  HandleScope scope(thread);
  ValueCell sys_stdout_cell(&scope, thread->runtime()->sysStdout());
  Object sys_stdout(&scope, NoneType::object());
  if (!sys_stdout_cell.isUnbound()) {
    sys_stdout = sys_stdout_cell.value();
  }
  writeImpl(thread, sys_stdout, stdout, format, va);
}

void writeStderr(Thread* thread, const char* format, ...) {
  va_list va;
  va_start(va, format);
  writeStderrV(thread, format, va);
  va_end(va);
}

void writeStderrV(Thread* thread, const char* format, va_list va) {
  HandleScope scope(thread);
  ValueCell sys_stderr_cell(&scope, thread->runtime()->sysStderr());
  Object sys_stderr(&scope, NoneType::object());
  if (!sys_stderr_cell.isUnbound()) {
    sys_stderr = sys_stderr_cell.value();
  }
  writeImpl(thread, sys_stderr, stderr, format, va);
}

const BuiltinMethod SysModule::kBuiltinMethods[] = {
    {SymbolId::kExcInfo, excInfo},
    {SymbolId::kExcepthook, excepthook},
    {SymbolId::kUnderGetframeCode, underGetframeCode},
    {SymbolId::kUnderGetframeGlobals, underGetframeGlobals},
    {SymbolId::kUnderGetframeLineno, underGetframeLineno},
    {SymbolId::kUnderGetframeLocals, underGetframeLocals},
    {SymbolId::kSentinelId, nullptr},
};

RawObject SysModule::excepthook(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  // type argument is ignored
  Object value(&scope, args.get(1));
  Object tb(&scope, args.get(2));
  displayException(thread, value, tb);
  return NoneType::object();
}

RawObject SysModule::excInfo(Thread* thread, Frame* /* frame */,
                             word /* nargs */) {
  HandleScope scope(thread);
  Tuple result(&scope, thread->runtime()->newTuple(3));
  if (thread->hasCaughtException()) {
    result.atPut(0, thread->caughtExceptionType());
    result.atPut(1, thread->caughtExceptionValue());
    result.atPut(2, thread->caughtExceptionTraceback());
  } else {
    result.atPut(0, RawNoneType::object());
    result.atPut(1, RawNoneType::object());
    result.atPut(2, RawNoneType::object());
  }
  return *result;
}

class UserVisibleFrameVisitor : public FrameVisitor {
 public:
  UserVisibleFrameVisitor(word depth) : target_depth_(depth) {}

  bool visit(Frame* frame) {
    if (current_depth_ == target_depth_) {
      target_ = frame;
      return false;
    }
    current_depth_++;
    return true;
  }

  Frame* target() { return target_; }

 private:
  word current_depth_ = 0;
  const word target_depth_;
  Frame* target_ = nullptr;
};

static Frame* frameAtDepth(Thread* thread, word depth) {
  UserVisibleFrameVisitor visitor(depth + 1);
  thread->visitFrames(&visitor);
  return visitor.target();
}

RawObject SysModule::underGetframeCode(Thread* thread, Frame* frame,
                                       word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object depth_obj(&scope, args.get(0));
  DCHECK(thread->runtime()->isInstanceOfInt(*depth_obj), "depth must be int");
  Int depth(&scope, intUnderlying(thread, depth_obj));
  if (depth.isNegative()) {
    return thread->raiseWithFmt(LayoutId::kValueError, "negative stack level");
  }
  frame = frameAtDepth(thread, depth.asWordSaturated());
  if (frame == nullptr) {
    return thread->raiseWithFmt(LayoutId::kValueError,
                                "call stack is not deep enough");
  }
  return frame->code();
}

RawObject SysModule::underGetframeGlobals(Thread* thread, Frame* frame,
                                          word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object depth_obj(&scope, args.get(0));
  DCHECK(thread->runtime()->isInstanceOfInt(*depth_obj), "depth must be int");
  Int depth(&scope, intUnderlying(thread, depth_obj));
  if (depth.isNegative()) {
    return thread->raiseWithFmt(LayoutId::kValueError, "negative stack level");
  }
  frame = frameAtDepth(thread, depth.asWordSaturated());
  if (frame == nullptr) {
    return thread->raiseWithFmt(LayoutId::kValueError,
                                "call stack is not deep enough");
  }
  Module module(&scope, frame->function().moduleObject());
  return module.moduleProxy();
}

RawObject SysModule::underGetframeLineno(Thread* thread, Frame* frame,
                                         word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object depth_obj(&scope, args.get(0));
  DCHECK(runtime->isInstanceOfInt(*depth_obj), "depth must be int");
  Int depth(&scope, intUnderlying(thread, depth_obj));
  if (depth.isNegative()) {
    return thread->raiseWithFmt(LayoutId::kValueError, "negative stack level");
  }
  frame = frameAtDepth(thread, depth.asWordSaturated());
  if (frame == nullptr) {
    return thread->raiseWithFmt(LayoutId::kValueError,
                                "call stack is not deep enough");
  }
  Code code(&scope, frame->code());
  word pc = frame->virtualPC();
  word lineno = thread->runtime()->codeOffsetToLineNum(thread, code, pc);
  return SmallInt::fromWord(lineno);
}

static RawObject dictOfLocalsFromFunctionFrame(Thread* thread, Frame* frame) {
  HandleScope scope(thread);
  Function function(&scope, frame->function());
  Code code(&scope, function.code());
  Runtime* runtime = thread->runtime();
  Tuple empty_tuple(&scope, runtime->emptyTuple());
  Tuple var_names(&scope,
                  code.varnames().isTuple() ? code.varnames() : *empty_tuple);
  Tuple freevar_names(
      &scope, code.freevars().isTuple() ? code.freevars() : *empty_tuple);
  Tuple cellvar_names(
      &scope, code.cellvars().isTuple() ? code.cellvars() : *empty_tuple);

  word var_names_length = var_names.length();
  word freevar_names_length = freevar_names.length();
  word cellvar_names_length = cellvar_names.length();

  DCHECK(function.totalLocals() ==
             var_names_length + freevar_names_length + cellvar_names_length,
         "numbers of local variables do not match");

  Dict result(&scope, runtime->newDict());
  Str name(&scope, Str::empty());
  Object value(&scope, NoneType::object());
  for (word i = 0; i < var_names_length; ++i) {
    name = var_names.at(i);
    value = frame->local(i);
    runtime->dictAtPutByStr(thread, result, name, value);
  }
  for (word i = 0, j = var_names_length; i < freevar_names_length; ++i, ++j) {
    name = freevar_names.at(i);
    DCHECK(frame->local(j).isValueCell(), "freevar must be ValueCell");
    value = ValueCell::cast(frame->local(j)).value();
    runtime->dictAtPutByStr(thread, result, name, value);
  }
  for (word i = 0, j = var_names_length + freevar_names_length;
       i < cellvar_names_length; ++i, ++j) {
    name = cellvar_names.at(i);
    DCHECK(frame->local(j).isValueCell(), "cellvar must be ValueCell");
    value = ValueCell::cast(frame->local(j)).value();
    runtime->dictAtPutByStr(thread, result, name, value);
  }
  return *result;
}

RawObject SysModule::underGetframeLocals(Thread* thread, Frame* frame,
                                         word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object depth_obj(&scope, args.get(0));
  DCHECK(runtime->isInstanceOfInt(*depth_obj), "depth must be int");
  Int depth(&scope, intUnderlying(thread, depth_obj));
  if (depth.isNegative()) {
    return thread->raiseWithFmt(LayoutId::kValueError, "negative stack level");
  }
  frame = frameAtDepth(thread, depth.asWordSaturated());
  if (frame == nullptr) {
    return thread->raiseWithFmt(LayoutId::kValueError,
                                "call stack is not deep enough");
  }
  Function function(&scope, frame->function());
  if (function.hasOptimizedOrNewLocals()) {
    return dictOfLocalsFromFunctionFrame(thread, frame);
  }
  Object implicit_globals_obj(&scope, frame->implicitGlobals());
  if (implicit_globals_obj == function.globals()) {
    // Module scope
    return frameGlobals(thread, frame);
  }
  // Other non-function scope (e.g., class scope)
  return *implicit_globals_obj;
}

}  // namespace python

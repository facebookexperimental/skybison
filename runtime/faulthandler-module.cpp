// Copyright (c) Facebook, Inc. and its affiliates. (http://www.facebook.com)
#include <signal.h>
#include <sys/resource.h>
#include <unistd.h>

#include <cerrno>
#include <csignal>

#include "builtins.h"
#include "file.h"
#include "int-builtins.h"
#include "modules.h"
#include "runtime.h"
#include "sys-module.h"

namespace py {

struct FaultHandler {
  int signum;
  const char* msg;
  bool enabled;
  struct sigaction previous_handler;
};

static struct {
  int fd;
  bool all_threads;
  bool enabled;
  Runtime* runtime;
} fatal_error = {-1};

static FaultHandler handler_sigabrt = {SIGABRT, "Aborted"};
static FaultHandler handler_sigbus = {SIGBUS, "Bus error"};
static FaultHandler handler_sigfpe = {SIGFPE, "Floating point exception"};
static FaultHandler handler_sigill = {SIGILL, "Illegal instruction"};
static FaultHandler handler_sigsegv = {SIGSEGV, "Segmentation fault"};

static void disableFatalHandler(FaultHandler* handler) {
  if (!handler->enabled) return;
  handler->enabled = false;
  int result =
      ::sigaction(handler->signum, &handler->previous_handler, nullptr);
  DCHECK(result == 0, "sigaction returned unexpected error");
}

static RawObject getFileno(Thread* thread, const Object& file) {
  Runtime* runtime = thread->runtime();
  if (file.isNoneType()) {
    return SmallInt::fromWord(File::kStderr);
  }
  if (runtime->isInstanceOfInt(*file)) {
    RawInt fd = intUnderlying(*file);
    if (fd.isNegative() || fd.isLargeInt()) {
      return thread->raiseWithFmt(LayoutId::kValueError,
                                  "file is not a valid file descriptor");
    }
    return fd.isSmallInt() ? fd : convertBoolToInt(fd);
  }

  HandleScope scope(thread);
  Object fileno(&scope, thread->invokeMethod1(file, ID(fileno)));
  if (fileno.isError()) {
    if (fileno.isErrorNotFound()) {
      return thread->raiseWithFmt(LayoutId::kAttributeError,
                                  "'%T' object has no attribute 'fileno'",
                                  &file);
    }
    return *fileno;
  }

  if (!runtime->isInstanceOfInt(*fileno)) {
    return thread->raiseWithFmt(LayoutId::kRuntimeError,
                                "file.fileno() is not a valid file descriptor");
  }
  Int fd(&scope, intUnderlying(*fileno));
  if (fd.isNegative() || fd.isLargeInt()) {
    return thread->raiseWithFmt(LayoutId::kRuntimeError,
                                "file.fileno() is not a valid file descriptor");
  }

  Object flush_result(&scope, thread->invokeMethod1(file, ID(flush)));
  if (flush_result.isErrorException()) {
    thread->clearPendingException();
  }
  return fd.isSmallInt() ? *fd : convertBoolToInt(*fd);
}

static void writeCStr(int fd, const char* str) {
  File::write(fd, str, std::strlen(str));
}

static void handleFatalError(FaultHandler* handler) {
  if (!fatal_error.enabled) return;

  int saved_errno = errno;
  disableFatalHandler(handler);

  int fd = fatal_error.fd;
  writeCStr(fd, "Fatal Python error: ");
  writeCStr(fd, handler->msg);
  writeCStr(fd, "\n\n");
  // TODO(T66337218): Print tracebacks for all threads when there is more than
  // one and`all_threads` is true.
  fatal_error.runtime->printTraceback(Thread::current(), fd);

  errno = saved_errno;
  std::raise(handler->signum);
}

static void handleSigabrt(int signum) {
  DCHECK(signum == SIGABRT, "expected SIGABRT (%d), got %d", SIGABRT, signum);
  handleFatalError(&handler_sigabrt);
}

static void handleSigbus(int signum) {
  DCHECK(signum == SIGBUS, "expected SIGBUS (%d), got %d", SIGBUS, signum);
  handleFatalError(&handler_sigbus);
}

static void handleSigfpe(int signum) {
  DCHECK(signum == SIGFPE, "expected SIGFPE (%d), got %d", SIGFPE, signum);
  handleFatalError(&handler_sigfpe);
}

static void handleSigill(int signum) {
  DCHECK(signum == SIGILL, "expected SIGILL (%d), got %d", SIGILL, signum);
  handleFatalError(&handler_sigill);
}

static void handleSigsegv(int signum) {
  DCHECK(signum == SIGSEGV, "expected SIGSEGV (%d), got %d", SIGSEGV, signum);
  handleFatalError(&handler_sigsegv);
}

// Disable creation of core dump
static void suppressCrashReport() {
  struct rlimit rl;
  if (::getrlimit(RLIMIT_CORE, &rl) == 0) {
    rl.rlim_cur = 0;
    ::setrlimit(RLIMIT_CORE, &rl);
  }
}

RawObject FUNC(faulthandler, _read_null)(Thread*, Arguments) {
  suppressCrashReport();
  *static_cast<volatile word*>(nullptr);
  return NoneType::object();
}

RawObject FUNC(faulthandler, _sigabrt)(Thread*, Arguments) {
  suppressCrashReport();
  std::abort();
  return NoneType::object();
}

RawObject FUNC(faulthandler, _sigfpe)(Thread*, Arguments) {
  suppressCrashReport();
  std::raise(SIGFPE);
  return NoneType::object();
}

RawObject FUNC(faulthandler, _sigsegv)(Thread*, Arguments) {
  suppressCrashReport();
  std::raise(SIGSEGV);
  return NoneType::object();
}

RawObject FUNC(faulthandler, disable)(Thread*, Arguments) {
  if (!fatal_error.enabled) {
    return Bool::falseObj();
  }

  fatal_error.enabled = false;
  disableFatalHandler(&handler_sigabrt);
  disableFatalHandler(&handler_sigbus);
  disableFatalHandler(&handler_sigill);
  disableFatalHandler(&handler_sigfpe);
  disableFatalHandler(&handler_sigsegv);
  return Bool::trueObj();
}

RawObject FUNC(faulthandler, dump_traceback)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object file(&scope, args.get(0));
  Object all_threads(&scope, args.get(1));

  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfInt(*all_threads)) {
    return thread->raiseRequiresType(all_threads, ID(int));
  }

  Object fileno(&scope, getFileno(thread, file));
  if (fileno.isError()) return *fileno;
  SmallInt fd(&scope, *fileno);
  // TODO(T66337218): Dump all threads when there is more than one and
  // `all_threads` is True.
  runtime->printTraceback(thread, fd.value());

  return runtime->handlePendingSignals(thread);
}

static bool enableHandler(FaultHandler* handler, void (*handler_func)(int)) {
  struct sigaction action;
  action.sa_handler = handler_func;
  sigemptyset(&action.sa_mask);
  // For GC-safety, we execute all signal handlers on an alternate stack.
  action.sa_flags = SA_NODEFER | SA_ONSTACK;

  if (::sigaction(handler->signum, &action, &handler->previous_handler) != 0) {
    return false;
  }

  handler->enabled = true;
  return true;
}

RawObject FUNC(faulthandler, enable)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object file(&scope, args.get(0));
  Object all_threads(&scope, args.get(1));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfInt(*all_threads)) {
    return thread->raiseRequiresType(all_threads, ID(int));
  }

  Object fileno(&scope, getFileno(thread, file));
  if (fileno.isError()) return *fileno;
  fatal_error.fd = SmallInt::cast(*fileno).value();
  fatal_error.all_threads = !intUnderlying(*all_threads).isZero();
  fatal_error.runtime = runtime;
  if (fatal_error.enabled) {
    return NoneType::object();
  }

  fatal_error.enabled = true;

  if (enableHandler(&handler_sigabrt, handleSigabrt) &&
      enableHandler(&handler_sigbus, handleSigbus) &&
      enableHandler(&handler_sigfpe, handleSigfpe) &&
      enableHandler(&handler_sigill, handleSigill) &&
      enableHandler(&handler_sigsegv, handleSigsegv)) {
    return NoneType::object();
  }

  int saved_errno = errno;
  const char* msg = std::strerror(saved_errno);
  MutableTuple val(&scope, runtime->newMutableTuple(2));
  val.atPut(0, SmallInt::fromWord(saved_errno));
  val.atPut(1, runtime->newStrFromCStr(msg));
  return thread->raise(LayoutId::kRuntimeError, val.becomeImmutable());
}

RawObject FUNC(faulthandler, is_enabled)(Thread*, Arguments) {
  return Bool::fromBool(fatal_error.enabled);
}

}  // namespace py

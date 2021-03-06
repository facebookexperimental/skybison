// Copyright (c) Facebook, Inc. and its affiliates. (http://www.facebook.com)
#include "os.h"

#include <dlfcn.h>
#include <pthread.h>
#include <sys/syscall.h>
#include <unistd.h>

#include <csignal>
#include <cstdlib>

#include "utils.h"

namespace py {

const word OS::kNumSignals = _NSIG;
bool volatile OS::pending_signals_[kNumSignals];

const int OS::kRtldGlobal = RTLD_GLOBAL;
const int OS::kRtldLocal = RTLD_LOCAL;
const int OS::kRtldNow = RTLD_NOW;

const char* OS::name() { return "linux"; }

// clang-format off
#define V(SIGNAL) { #SIGNAL, SIGNAL }
const OS::Signal OS::kPlatformSignals[] = {
    V(SIGCLD),
    V(SIGIO),
    V(SIGIOT),
    V(SIGPOLL),
    V(SIGPROF),
    V(SIGPWR),
    V(SIGRTMAX),
    V(SIGRTMIN),
    V(SIGSYS),
    V(SIGVTALRM),
    V(SIGWINCH),
    { nullptr, 0 },
};
#undef V
// clang-format on

void OS::createThread(ThreadFunction func, void* arg) {
  pthread_t thread;
  pthread_create(&thread, nullptr, func, arg);
  pthread_detach(thread);
}

char* OS::executablePath() {
  char* buffer = readLink("/proc/self/exe");
  CHECK(buffer != nullptr, "failed to determine executable path");
  return buffer;
}

void* OS::openSharedObject(const char* filename, int mode,
                           const char** error_msg) {
  void* result = ::dlopen(filename, mode);
  if (result == nullptr) {
    *error_msg = ::dlerror();
  }
  return result;
}

SignalHandler OS::setSignalHandler(int signum, SignalHandler handler) {
  struct sigaction new_context, old_context;
  new_context.sa_handler = handler;
  sigemptyset(&new_context.sa_mask);
  new_context.sa_flags = 0;
  if (::sigaction(signum, &new_context, &old_context) == -1) {
    return SIG_ERR;
  }
  return old_context.sa_handler;
}

SignalHandler OS::signalHandler(int signum) {
  struct sigaction context;
  if (::sigaction(signum, nullptr, &context) == -1) {
    return SIG_ERR;
  }
  return context.sa_handler;
}

void* OS::sharedObjectSymbolAddress(void* handle, const char* symbol,
                                    const char** error_msg) {
  void* result = ::dlsym(handle, symbol);
  if (result == nullptr && error_msg != nullptr) {
    *error_msg = ::dlerror();
  }
  return result;
}

word OS::sharedObjectSymbolName(void* addr, char* buf, word size) {
  Dl_info info;
  if (::dladdr(addr, &info) && info.dli_sname != nullptr) {
    return std::snprintf(buf, size, "%s", info.dli_sname);
  }
  return -1;
}

}  // namespace py

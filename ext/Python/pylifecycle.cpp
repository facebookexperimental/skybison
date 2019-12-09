#include <unistd.h>
#include <cstdio>

#include "capi-handles.h"
#include "cpython-func.h"
#include "exception-builtins.h"
#include "runtime.h"

// TODO(T57880525): Reconcile these flags with sys.py
int Py_BytesWarningFlag = 0;
int Py_DebugFlag = 0;
int Py_DontWriteBytecodeFlag = 0;
int Py_FrozenFlag = 0;
int Py_HashRandomizationFlag = 0;
int Py_IgnoreEnvironmentFlag = 0;
int Py_InspectFlag = 0;
int Py_InteractiveFlag = 0;
int Py_IsolatedFlag = 0;
int Py_NoSiteFlag = 0;
int Py_NoUserSiteDirectory = 0;
int Py_OptimizeFlag = 0;
int Py_QuietFlag = 0;
int Py_UnbufferedStdioFlag = 0;
int Py_UseClassExceptionsFlag = 1;
int Py_VerboseFlag = 0;

namespace py {

PY_EXPORT PyOS_sighandler_t PyOS_getsig(int /* g */) {
  UNIMPLEMENTED("PyOS_getsig");
}

PY_EXPORT PyOS_sighandler_t PyOS_setsig(int /* g */,
                                        PyOS_sighandler_t /* r */) {
  UNIMPLEMENTED("PyOS_setsig");
}

PY_EXPORT int Py_AtExit(void (*/* func */)(void)) {
  UNIMPLEMENTED("Py_AtExit");
}

PY_EXPORT void Py_EndInterpreter(PyThreadState* /* e */) {
  UNIMPLEMENTED("Py_EndInterpreter");
}

PY_EXPORT void Py_Exit(int /* s */) { UNIMPLEMENTED("Py_Exit"); }

PY_EXPORT void Py_FatalError(const char* msg) {
  // TODO(T39151288): Correctly print exceptions when the current thread holds
  // the GIL
  std::fprintf(stderr, "Fatal Python error: %s\n", msg);
  Thread* thread = Thread::current();
  if (thread != nullptr) {
    if (thread->hasPendingException()) {
      printPendingException(thread);
    } else {
      Utils::printTracebackToStderr();
    }
  }
  std::abort();
}

// The file descriptor fd is considered ``interactive'' if either:
//   a) isatty(fd) is TRUE, or
//   b) the -i flag was given, and the filename associated with the descriptor
//      is NULL or "<stdin>" or "???".
PY_EXPORT int Py_FdIsInteractive(FILE* fp, const char* filename) {
  if (::isatty(fileno(fp))) {
    return 1;
  }
  if (!Py_InteractiveFlag) {
    return 0;
  }
  return filename == nullptr || std::strcmp(filename, "<stdin>") == 0 ||
         std::strcmp(filename, "???") == 0;
}

PY_EXPORT void Py_Finalize() { Py_FinalizeEx(); }

PY_EXPORT int Py_FinalizeEx() {
  Thread* thread = Thread::current();
  Runtime* runtime = thread->runtime();
  delete runtime;
  return 0;
}

PY_EXPORT void Py_Initialize() { Py_InitializeEx(1); }

PY_EXPORT void Py_InitializeEx(int initsigs) {
  CHECK(initsigs == 1, "Skipping signal handler registration unimplemented");
  // TODO(T55262429): Reduce the heap size once memory issues are fixed.
  new Runtime(128 * kMiB);
}

PY_EXPORT int Py_IsInitialized() { UNIMPLEMENTED("Py_IsInitialized"); }

PY_EXPORT PyThreadState* Py_NewInterpreter() {
  UNIMPLEMENTED("Py_NewInterpreter");
}

PY_EXPORT wchar_t* Py_GetProgramName() { UNIMPLEMENTED("Py_GetProgramName"); }

PY_EXPORT wchar_t* Py_GetPythonHome() { UNIMPLEMENTED("Py_GetPythonHome"); }

PY_EXPORT void Py_SetProgramName(wchar_t* /* name */) {
  UNIMPLEMENTED("Py_SetProgramName");
}

PY_EXPORT void Py_SetPythonHome(wchar_t* /* home */) {
  UNIMPLEMENTED("Py_SetPythonHome");
}

PY_EXPORT void _Py_PyAtExit(void (*func)(void)) {
  Thread::current()->runtime()->setAtExit(func);
}

PY_EXPORT void _Py_RestoreSignals() { UNIMPLEMENTED("_Py_RestoreSignals"); }

}  // namespace py

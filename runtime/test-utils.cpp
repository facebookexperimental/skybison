#include "test-utils.h"

#include <memory>
#include <sstream>

#include "builtins-module.h"
#include "frame.h"
#include "handles.h"
#include "runtime.h"
#include "thread.h"
#include "utils.h"

namespace python {
namespace testing {

// Turn a Python string object into a standard string.
static std::string pyStringToStdString(Str* pystr) {
  std::string std_str;
  std_str.reserve(pystr->length());
  for (word i = 0; i < pystr->length(); i++) {
    std_str += pystr->charAt(i);
  }
  return std_str;
}

::testing::AssertionResult AssertPyStringEqual(const char* actual_string_expr,
                                               const char*, Str* actual_str,
                                               std::string expected_string) {
  if (actual_str->equalsCStr(expected_string.c_str())) {
    return ::testing::AssertionSuccess();
  }

  return ::testing::AssertionFailure()
         << "      Expected: " << actual_string_expr << "\n"
         << "      Which is: \"" << pyStringToStdString(actual_str) << "\"\n"
         << "To be equal to: \"" << expected_string << "\"";
}

::testing::AssertionResult AssertPyStringEqual(const char* actual_string_expr,
                                               const char* expected_string_expr,
                                               Str* actual_str,
                                               Str* expected_str) {
  if (actual_str->equals(expected_str)) {
    return ::testing::AssertionSuccess();
  }

  return ::testing::AssertionFailure()
         << "      Expected: " << actual_string_expr << "\n"
         << "      Which is: \"" << pyStringToStdString(actual_str) << "\"\n"
         << "To be equal to: \"" << expected_string_expr << "\"\n"
         << "      Which is: \"" << pyStringToStdString(expected_str) << "\"";
}

// helper function to redirect return stdout/stderr from running a module
static std::string compileAndRunImpl(Runtime* runtime, const char* src,
                                     std::ostream** ostream) {
  std::stringstream tmp_ostream;
  std::ostream* saved_ostream = *ostream;
  *ostream = &tmp_ostream;
  Object* result = runtime->runFromCStr(src);
  (void)result;
  CHECK(result == NoneType::object(), "unexpected result");
  *ostream = saved_ostream;
  return tmp_ostream.str();
}

std::string compileAndRunToString(Runtime* runtime, const char* src) {
  return compileAndRunImpl(runtime, src, &builtInStdout);
}

std::string compileAndRunToStderrString(Runtime* runtime, const char* src) {
  return compileAndRunImpl(runtime, src, &builtinStderr);
}

std::string callFunctionToString(const Handle<Function>& func,
                                 const Handle<ObjectArray>& args) {
  std::stringstream stream;
  std::ostream* old_stream = builtInStdout;
  builtInStdout = &stream;
  Thread* thread = Thread::currentThread();
  Frame* frame =
      thread->pushNativeFrame(Utils::castFnPtrToVoid(callFunctionToString), 0);
  callFunction(func, args);
  thread->popFrame();
  builtInStdout = old_stream;
  return stream.str();
}

Object* callFunction(const Handle<Function>& func,
                     const Handle<ObjectArray>& args) {
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Handle<Code> code(&scope, func->code());
  Frame* frame = thread->pushNativeFrame(Utils::castFnPtrToVoid(callFunction),
                                         args->length());
  frame->pushValue(*func);
  for (word i = 0; i < args->length(); i++) {
    frame->pushValue(args->at(i));
  }
  Handle<Object> result(&scope, func->entry()(thread, frame, code->argcount()));
  thread->popFrame();
  return *result;
}

bool objectArrayContains(const Handle<ObjectArray>& object_array,
                         const Handle<Object>& key) {
  for (word i = 0; i < object_array->length(); i++) {
    if (Object::equals(object_array->at(i), *key)) {
      return true;
    }
  }
  return false;
}

Object* findModule(Runtime* runtime, const char* name) {
  HandleScope scope;
  Handle<Object> key(&scope, runtime->newStrFromCStr(name));
  return runtime->findModule(key);
}

Object* moduleAt(Runtime* runtime, const Handle<Module>& module,
                 const char* name) {
  HandleScope scope;
  Handle<Object> key(&scope, runtime->newStrFromCStr(name));
  return runtime->moduleAt(module, key);
}

Object* moduleAt(Runtime* runtime, const char* module_name, const char* name) {
  HandleScope scope;
  Handle<Object> mod_obj(&scope, findModule(runtime, module_name));
  if (mod_obj->isNoneType()) {
    return Error::object();
  }
  Handle<Module> module(mod_obj);
  return moduleAt(runtime, module, name);
}

std::string typeName(Runtime* runtime, Object* obj) {
  Str* name = Str::cast(Type::cast(runtime->typeOf(obj))->name());
  word length = name->length();
  std::string result(length, '\0');
  name->copyTo(reinterpret_cast<byte*>(&result[0]), length);
  return result;
}

}  // namespace testing
}  // namespace python

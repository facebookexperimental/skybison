#include "test-utils.h"

#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <sstream>

#include "builtins-module.h"
#include "bytearray-builtins.h"
#include "bytes-builtins.h"
#include "debugging.h"
#include "frame.h"
#include "handles.h"
#include "os.h"
#include "runtime.h"
#include "thread.h"
#include "utils.h"

namespace python {

namespace testing {

Value::Type Value::type() const { return type_; }

bool Value::boolVal() const {
  DCHECK(type() == Type::Bool, "expected bool");
  return bool_;
}

word Value::intVal() const {
  DCHECK(type() == Type::Int, "expected int");
  return int_;
}

double Value::floatVal() const {
  DCHECK(type() == Type::Float, "expected float");
  return float_;
}

const char* Value::strVal() const {
  DCHECK(type() == Type::Str, "expected str");
  return str_;
}

template <typename T1, typename T2>
::testing::AssertionResult badListValue(const char* actual_expr, word i,
                                        const T1& actual, const T2& expected) {
  return ::testing::AssertionFailure()
         << "Value of: " << actual_expr << '[' << i << "]\n"
         << "  Actual: " << actual << '\n'
         << "Expected: " << expected;
}

::testing::AssertionResult AssertPyListEqual(
    const char* actual_expr, const char* /* expected_expr */,
    const Object& actual, const std::vector<Value>& expected) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();

  if (!actual.isList()) {
    return ::testing::AssertionFailure()
           << " Type of: " << actual_expr << "\n"
           << "  Actual: " << typeName(runtime, *actual) << "\n"
           << "Expected: list";
  }

  HandleScope scope(thread);
  List list(&scope, *actual);
  if (static_cast<size_t>(list.numItems()) != expected.size()) {
    return ::testing::AssertionFailure()
           << "Length of: " << actual_expr << "\n"
           << "   Actual: " << list.numItems() << "\n"
           << " Expected: " << expected.size();
  }

  for (size_t i = 0; i < expected.size(); i++) {
    Object actual_item(&scope, list.at(i));
    const Value& expected_item = expected[i];

    auto bad_type = [&](const char* expected_type) {
      return ::testing::AssertionFailure()
             << " Type of: " << actual_expr << '[' << i << "]\n"
             << "  Actual: " << typeName(runtime, *actual_item) << '\n'
             << "Expected: " << expected_type;
    };

    switch (expected_item.type()) {
      case Value::Type::None: {
        if (!actual_item.isNoneType()) return bad_type("RawNoneType");
        break;
      }

      case Value::Type::Bool: {
        if (!actual_item.isBool()) return bad_type("bool");
        auto const actual_val = RawBool::cast(*actual_item) == Bool::trueObj();
        auto const expected_val = expected_item.boolVal();
        if (actual_val != expected_val) {
          return badListValue(actual_expr, i, actual_val ? "True" : "False",
                              expected_val ? "True" : "False");
        }
        break;
      }

      case Value::Type::Int: {
        if (!actual_item.isInt()) return bad_type("int");
        Int actual_val(&scope, *actual_item);
        Int expected_val(&scope, runtime->newInt(expected_item.intVal()));
        if (actual_val.compare(*expected_val) != 0) {
          // TODO(bsimmers): Support multi-digit values when we can print them.
          return badListValue(actual_expr, i, actual_val.digitAt(0),
                              expected_item.intVal());
        }
        break;
      }

      case Value::Type::Float: {
        if (!actual_item.isFloat()) return bad_type("float");
        auto const actual_val = RawFloat::cast(*actual_item)->value();
        auto const expected_val = expected_item.floatVal();
        if (std::abs(actual_val - expected_val) >= DBL_EPSILON) {
          return badListValue(actual_expr, i, actual_val, expected_val);
        }
        break;
      }

      case Value::Type::Str: {
        if (!actual_item.isStr()) return bad_type("str");
        Str actual_val(&scope, *actual_item);
        const char* expected_val = expected_item.strVal();
        if (!actual_val.equalsCStr(expected_val)) {
          return badListValue(actual_expr, i, actual_val, expected_val);
        }
        break;
      }
    }
  }

  return ::testing::AssertionSuccess();
}

// helper function to redirect return stdout/stderr from running a module
static std::string compileAndRunImpl(Runtime* runtime, const char* src,
                                     std::ostream** ostream) {
  std::stringstream tmp_ostream;
  std::ostream* saved_ostream = *ostream;
  *ostream = &tmp_ostream;
  RawObject result = runFromCStr(runtime, src);
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

std::string callFunctionToString(const Function& func, const Tuple& args) {
  std::stringstream stream;
  std::ostream* old_stream = builtInStdout;
  builtInStdout = &stream;
  Thread* thread = Thread::currentThread();
  thread->pushNativeFrame(bit_cast<void*>(&callFunctionToString), 0);
  callFunction(func, args);
  thread->popFrame();
  builtInStdout = old_stream;
  return stream.str();
}

RawObject callFunction(const Function& func, const Tuple& args) {
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Code code(&scope, func.code());
  Frame* frame =
      thread->pushNativeFrame(bit_cast<void*>(&callFunction), args.length());
  frame->pushValue(*func);
  for (word i = 0; i < args.length(); i++) {
    frame->pushValue(args.at(i));
  }
  Object result(&scope, func.entry()(thread, frame, code.argcount()));
  thread->popFrame();
  return *result;
}

bool tupleContains(const Tuple& object_array, const Object& key) {
  for (word i = 0; i < object_array.length(); i++) {
    if (Object::equals(object_array.at(i), *key)) {
      return true;
    }
  }
  return false;
}

RawObject findModule(Runtime* runtime, const char* name) {
  HandleScope scope;
  Object key(&scope, runtime->newStrFromCStr(name));
  return runtime->findModule(key);
}

RawObject moduleAt(Runtime* runtime, const Module& module, const char* name) {
  HandleScope scope;
  Object key(&scope, runtime->newStrFromCStr(name));
  return runtime->moduleAt(module, key);
}

RawObject moduleAt(Runtime* runtime, const char* module_name,
                   const char* name) {
  HandleScope scope;
  Object mod_obj(&scope, findModule(runtime, module_name));
  if (mod_obj.isNoneType()) {
    return Error::object();
  }
  Module module(&scope, *mod_obj);
  return moduleAt(runtime, module, name);
}

std::string typeName(Runtime* runtime, RawObject obj) {
  if (obj.layoutId() == LayoutId::kError) return "Error";
  RawStr name = RawStr::cast(RawType::cast(runtime->typeOf(obj))->name());
  word length = name->length();
  std::string result(length, '\0');
  name->copyTo(reinterpret_cast<byte*>(&result[0]), length);
  return result;
}

RawObject newIntWithDigits(Runtime* runtime, View<uword> digits) {
  return runtime->newIntWithDigits(View<uword>(digits.data(), digits.length()));
}

RawLargeInt newLargeIntWithDigits(View<uword> digits) {
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  LargeInt result(&scope,
                  thread->runtime()->heap()->createLargeInt(digits.length()));
  for (word i = 0, e = digits.length(); i < e; ++i) {
    result.digitAtPut(i, digits.get(i));
  }
  return *result;
}

RawObject setFromRange(word start, word stop) {
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Set result(&scope, thread->runtime()->newSet());
  Object value(&scope, NoneType::object());
  for (word i = start; i < stop; i++) {
    value = SmallInt::fromWord(i);
    thread->runtime()->setAdd(result, value);
  }
  return *result;
}

RawObject runBuiltinImpl(NativeMethodType method,
                         View<std::reference_wrapper<const Object>> args) {
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  // Push an empty function so we have one at the expected place in the stack.
  Function function(&scope, thread->runtime()->newFunction());
  thread->currentFrame()->pushValue(*function);

  Frame* frame = thread->openAndLinkFrame(0, args.length(), 0);
  for (word i = 0; i < args.length(); i++) {
    frame->setLocal(i, *args.get(i).get());
  }
  Object result(&scope, method(thread, frame, args.length()));
  thread->popFrame();
  return *result;
}

RawObject runBuiltin(NativeMethodType method) {
  return runBuiltinImpl(method,
                        View<std::reference_wrapper<const Object>>{nullptr, 0});
}

RawObject runFromCStr(Runtime* runtime, const char* c_str) {
  return runtime->run(Runtime::compileFromCStr(c_str).get());
}

// Equivalent to evaluating "list(range(start, stop))" in Python
RawObject listFromRange(word start, word stop) {
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  List result(&scope, thread->runtime()->newList());
  Object value(&scope, NoneType::object());
  for (word i = start; i < stop; i++) {
    value = SmallInt::fromWord(i);
    thread->runtime()->listAdd(result, value);
  }
  return *result;
}

::testing::AssertionResult isBytesEqualsBytes(const Object& result,
                                              View<byte> expected) {
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  if (thread->hasPendingException()) {
    Type type(&scope, thread->pendingExceptionType());
    return ::testing::AssertionFailure()
           << "pending '" << typeName(runtime, *type) << "' exception";
  }
  if (!runtime->isInstanceOfBytes(*result)) {
    return ::testing::AssertionFailure()
           << "is a '" << typeName(runtime, *result) << "'";
  }
  Bytes result_bytes(&scope, *result);
  Bytes expected_bytes(&scope, runtime->newBytesWithAll(expected));
  if (result_bytes.compare(*expected_bytes) != 0) {
    Str result_repr(&scope, bytesReprSmartQuotes(thread, result_bytes));
    Str expected_repr(&scope, bytesReprSmartQuotes(thread, expected_bytes));
    return ::testing::AssertionFailure()
           << result_repr.toCStr() << "is not equal to "
           << expected_repr.toCStr();
  }
  return ::testing::AssertionSuccess();
}

::testing::AssertionResult isBytesEqualsCStr(const Object& result,
                                             const char* expected) {
  return isBytesEqualsBytes(
      result, View<byte>(reinterpret_cast<const byte*>(expected),
                         static_cast<word>(std::strlen(expected))));
}

::testing::AssertionResult isStrEquals(const Object& str1, const Object& str2) {
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfStr(*str1)) {
    return ::testing::AssertionFailure()
           << "is a '" << typeName(runtime, *str1) << "'";
  }
  if (!runtime->isInstanceOfStr(*str2)) {
    return ::testing::AssertionFailure()
           << "is a '" << typeName(runtime, *str2) << "'";
  }
  Str s1(&scope, *str1);
  if (!s1.equals(*str2)) {
    Str s2(&scope, *str2);
    return ::testing::AssertionFailure()
           << "is not equal to '" << s2.toCStr() << "'";
  }
  return ::testing::AssertionSuccess();
}

::testing::AssertionResult isStrEqualsCStr(RawObject obj, const char* c_str) {
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Object str_obj(&scope, obj);
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfStr(*str_obj)) {
    return ::testing::AssertionFailure()
           << "is a '" << typeName(runtime, *str_obj) << "'";
  }
  Str str(&scope, *str_obj);
  if (!str.equalsCStr(c_str)) {
    return ::testing::AssertionFailure()
           << "'" << str.toCStr() << "' is not equal to '" << c_str << "'";
  }
  return ::testing::AssertionSuccess();
}

::testing::AssertionResult isIntEqualsWord(RawObject obj, word value) {
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  if (obj.isError()) {
    if (thread->hasPendingException()) {
      Type type(&scope, thread->pendingExceptionType());
      Str type_name(&scope, type.name());
      return ::testing::AssertionFailure()
             << "pending " << type_name << " exception";
    }
    return ::testing::AssertionFailure() << "is an Error";
  }
  if (!runtime->isInstanceOfInt(obj)) {
    return ::testing::AssertionFailure()
           << "is a '" << typeName(runtime, obj) << "'";
  }
  Int value_int(&scope, obj);
  if (value_int.numDigits() > 1 || value_int.asWord() != value) {
    return ::testing::AssertionFailure()
           << value_int << " is not equal to " << value;
  }
  return ::testing::AssertionSuccess();
}

::testing::AssertionResult isIntEqualsDigits(RawObject obj,
                                             View<uword> digits) {
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  if (obj.isError()) {
    if (thread->hasPendingException()) {
      Type type(&scope, thread->pendingExceptionType());
      Str type_name(&scope, type.name());
      return ::testing::AssertionFailure()
             << "pending " << type_name << " exception";
    }
    return ::testing::AssertionFailure() << "is an Error";
  }
  if (!runtime->isInstanceOfInt(obj)) {
    return ::testing::AssertionFailure()
           << "is a '" << typeName(runtime, obj) << "'";
  }
  Int expected(&scope, newIntWithDigits(runtime, digits));
  Int value_int(&scope, obj);
  if (expected.compare(*value_int) != 0) {
    return ::testing::AssertionFailure()
           << value_int << " is not equal to " << expected;
  }
  return ::testing::AssertionSuccess();
}

::testing::AssertionResult raised(RawObject return_value, LayoutId layout_id) {
  return raisedWithStr(return_value, layout_id, nullptr);
}

::testing::AssertionResult raisedWithStr(RawObject return_value,
                                         LayoutId layout_id,
                                         const char* expected) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Object return_value_obj(&scope, return_value);

  if (!return_value_obj.isError()) {
    Type type(&scope, runtime->typeOf(*return_value_obj));
    Str name(&scope, type.name());
    return ::testing::AssertionFailure()
           << "call returned " << name << ", not Error";
  }

  if (!thread->hasPendingException()) {
    return ::testing::AssertionFailure() << "no exception pending";
  }

  Type expected_type(&scope, runtime->typeAt(layout_id));
  Type exception_type(&scope, thread->pendingExceptionType());
  if (!runtime->isSubclass(exception_type, expected_type)) {
    Str expected_name(&scope, expected_type.name());
    Str actual_name(&scope, exception_type.name());
    return ::testing::AssertionFailure()
           << "\npending exception has type:\n  " << actual_name
           << "\nexpected:\n  " << expected_name << "\n";
  }

  if (expected == nullptr) return ::testing::AssertionSuccess();

  Object exc_value(&scope, thread->pendingExceptionValue());
  if (!runtime->isInstanceOfStr(*exc_value)) {
    if (runtime->isInstanceOfBaseException(*exc_value)) {
      BaseException exc(&scope, *exc_value);
      Tuple args(&scope, exc.args());
      if (args.length() == 0) {
        return ::testing::AssertionFailure()
               << "pending exception args tuple is empty";
      }
      exc_value = args.at(0);
    }

    if (!runtime->isInstanceOfStr(*exc_value)) {
      return ::testing::AssertionFailure()
             << "pending exception value is not str";
    }
  }

  Str exc_msg(&scope, *exc_value);
  if (!exc_msg.equalsCStr(expected)) {
    return ::testing::AssertionFailure()
           << "\npending exception value:\n  '" << exc_msg
           << "'\nexpected:\n  '" << expected << "'\n";
  }

  return ::testing::AssertionSuccess();
}

}  // namespace testing
}  // namespace python

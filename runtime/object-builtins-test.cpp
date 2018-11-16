#include "gtest/gtest.h"

#include "frame.h"
#include "object-builtins.h"
#include "runtime.h"
#include "test-utils.h"

namespace python {

using namespace testing;

TEST(ObjectBuiltinsTest, ObjectDunderReprReturnsTypeNameAndPointer) {
  Runtime runtime;
  runtime.runFromCStr(R"(
class Foo:
  pass

a = object.__repr__(Foo())
)");
  HandleScope scope;
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Str> a(&scope, moduleAt(&runtime, main, "a"));
  // Storage for the class name. It must be shorter than the length of the whole
  // string.
  char* c_str = a->toCStr();
  char* class_name = static_cast<char*>(std::malloc(a->length()));
  void* ptr = nullptr;
  int num_written = std::sscanf(c_str, "<%s object at %p>", class_name, &ptr);
  ASSERT_EQ(num_written, 2);
  EXPECT_STREQ(class_name, "Foo");
  // No need to check the actual pointer value, just make sure something was
  // there.
  EXPECT_NE(ptr, nullptr);
  free(class_name);
  free(c_str);
}

TEST(ObjectBuiltinsTest, DunderStrReturnsDunderRepr) {
  Runtime runtime;
  runtime.runFromCStr(R"(
class Foo:
  pass

f = Foo()
a = object.__str__(f)
b = object.__repr__(f)
)");
  HandleScope scope;
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Str> a(&scope, moduleAt(&runtime, main, "a"));
  Handle<Str> b(&scope, moduleAt(&runtime, main, "b"));
  EXPECT_PYSTRING_EQ(*a, *b);
}

TEST(ObjectBuiltinsTest, UserDefinedTypeInheritsDunderStr) {
  Runtime runtime;
  runtime.runFromCStr(R"(
class Foo:
  pass

f = Foo()
a = object.__str__(f)
b = f.__str__()
)");
  HandleScope scope;
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Str> a(&scope, moduleAt(&runtime, main, "a"));
  Handle<Str> b(&scope, moduleAt(&runtime, main, "b"));
  EXPECT_PYSTRING_EQ(*a, *b);
}

TEST(ObjectBuiltinsTest, DunderInitDoesNotThrowIfNewIsDifferentButInitIsSame) {
  Runtime runtime;
  runtime.runFromCStr(R"(
class Foo:
  def __new__(cls):
    return object.__new__(cls)

Foo.__init__(Foo(), 1)
)");
  // It doesn't matter what the output is, just that it doesn't throw a
  // TypeError.
}

TEST(ObjectBuiltinsTest, DunderInitWithNonInstanceIsOk) {
  Runtime runtime;
  runtime.runFromCStr(R"(
object.__init__(object)
)");
  // It doesn't matter what the output is, just that it doesn't throw a
  // TypeError.
}

TEST(ObjectBuiltinsDeathTest, DunderInitWithNoArgsThrowsTypeError) {
  Runtime runtime;
  const char* src = R"(
object.__init__()
)";
  // Passing no args to object.__init__ should throw a type error.
  EXPECT_DEATH(runtime.runFromCStr(src), "aborting due to pending exception");
}

TEST(ObjectBuiltinsDeathTest, DunderInitWithArgsThrowsTypeError) {
  Runtime runtime;
  // Passing extra args to object.__init__, without overwriting __new__,
  // should throw a type error.
  const char* src = R"(
class Foo:
  pass

Foo.__init__(Foo(), 1)
)";
  EXPECT_DEATH(runtime.runFromCStr(src), "aborting due to pending exception");
}

TEST(ObjectBuiltinsDeathTest, DunderInitWithNewAndInitThrowsTypeError) {
  Runtime runtime;
  // Passing extra args to object.__init__, and overwriting only __init__,
  // should throw a type error.
  const char* src = R"(
class Foo:
  def __init__(self):
    object.__init__(self, 1)

Foo()
)";
  EXPECT_DEATH(runtime.runFromCStr(src), "aborting due to pending exception");
}

TEST(NoneBuiltinsTest, NewReturnsNone) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  Frame* frame = thread->openAndLinkFrame(0, 1, 0);
  frame->setLocal(0, runtime.typeAt(LayoutId::kNoneType));
  EXPECT_TRUE(NoneBuiltins::dunderNew(thread, frame, 1)->isNoneType());
}

TEST(NoneBuiltinsTest, NewWithExtraArgsThrows) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  Frame* frame = thread->openAndLinkFrame(0, 4, 0);
  frame->setLocal(0, runtime.typeAt(LayoutId::kNoneType));
  frame->setLocal(1, runtime.newInt(1));
  frame->setLocal(2, runtime.newInt(2));
  frame->setLocal(3, runtime.newInt(3));
  EXPECT_TRUE(NoneBuiltins::dunderNew(thread, frame, 4)->isError());
}

}  // namespace python

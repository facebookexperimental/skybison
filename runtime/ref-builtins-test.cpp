#include "gtest/gtest.h"

#include "ref-builtins.h"
#include "runtime.h"
#include "super-builtins.h"
#include "test-utils.h"

namespace python {

using namespace testing;

using RefBuiltinsTest = RuntimeFixture;

TEST_F(RefBuiltinsTest, ReferentTest) {
  const char* src = R"(
from _weakref import ref
class Foo: pass
a = Foo()
weak = ref(a)
)";
  HandleScope scope(thread_);
  compileAndRunToString(&runtime_, src);
  RawObject a = moduleAt(&runtime_, "__main__", "a");
  RawObject weak = moduleAt(&runtime_, "__main__", "weak");
  EXPECT_EQ(WeakRef::cast(weak).referent(), a);
  EXPECT_EQ(WeakRef::cast(weak).callback(), NoneType::object());

  Module main(&scope, findModule(&runtime_, "__main__"));
  Dict globals(&scope, main.dict());
  Object key(&scope, runtime_.newStrFromCStr("a"));
  runtime_.dictRemove(thread_, globals, key);

  runtime_.collectGarbage();
  weak = moduleAt(&runtime_, "__main__", "weak");
  EXPECT_EQ(WeakRef::cast(weak).referent(), NoneType::object());
}

TEST_F(RefBuiltinsTest, CallbackTest) {
  const char* src = R"(
from _weakref import ref
class Foo: pass
a = Foo()
b = 1
def f(ref):
    global b
    b = 2
weak = ref(a, f)
)";
  HandleScope scope(thread_);
  compileAndRunToString(&runtime_, src);
  RawObject a = moduleAt(&runtime_, "__main__", "a");
  RawObject b = moduleAt(&runtime_, "__main__", "b");
  RawObject weak = moduleAt(&runtime_, "__main__", "weak");
  EXPECT_EQ(WeakRef::cast(weak).referent(), a);
  EXPECT_TRUE(isIntEqualsWord(b, 1));

  Module main(&scope, findModule(&runtime_, "__main__"));
  Dict globals(&scope, main.dict());
  Object key(&scope, runtime_.newStrFromCStr("a"));
  runtime_.dictRemove(thread_, globals, key);

  runtime_.collectGarbage();
  weak = moduleAt(&runtime_, "__main__", "weak");
  b = moduleAt(&runtime_, "__main__", "b");
  EXPECT_TRUE(isIntEqualsWord(b, 2));
  EXPECT_EQ(WeakRef::cast(weak).referent(), NoneType::object());
  EXPECT_EQ(WeakRef::cast(weak).callback(), NoneType::object());
}

TEST_F(RefBuiltinsTest, DunderCallReturnsObject) {
  HandleScope scope(thread_);
  Object obj(&scope, Str::empty());
  Object callback(&scope, NoneType::object());
  WeakRef ref(&scope, runtime_.newWeakRef(thread_, obj, callback));
  Object result(&scope, runBuiltin(RefBuiltins::dunderCall, ref));
  EXPECT_EQ(result, obj);
}

TEST_F(RefBuiltinsTest, DunderCallWithNonRefRaisesTypeError) {
  HandleScope scope;
  Object obj(&scope, NoneType::object());
  Object result(&scope, runBuiltin(RefBuiltins::dunderCall, obj));
  EXPECT_TRUE(raisedWithStr(*result, LayoutId::kTypeError,
                            "'__call__' requires a 'ref' object"));
}

TEST_F(RefBuiltinsTest, DunderHashWithDeadRefRaisesTypeError) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
import _weakref
class C:
  pass
ref = _weakref.ref(C())
)")
                   .isError());
  runtime_.collectGarbage();
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, "ref.__hash__()"),
                            LayoutId::kTypeError, "weak object has gone away"));
}

TEST_F(RefBuiltinsTest, DunderHashCallsHashOfReferent) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
import _weakref
class C:
  def __hash__(self):
    raise Exception("foo")
c = C()
ref = _weakref.ref(c)
)")
                   .isError());
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, "ref.__hash__()"),
                            LayoutId::kException, "foo"));
}

}  // namespace python

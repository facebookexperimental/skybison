#include "gtest/gtest.h"

#include "builtins-module.h"
#include "scavenger.h"
#include "test-utils.h"
#include "trampolines.h"

namespace python {
using namespace testing;

using ScavengerTest = RuntimeFixture;

TEST_F(ScavengerTest, PreserveWeakReferenceHeapReferent) {
  HandleScope scope(thread_);
  Tuple array(&scope, runtime_.newTuple(10));
  Object none(&scope, NoneType::object());
  WeakRef ref(&scope, runtime_.newWeakRef(thread_, array, none));
  runtime_.collectGarbage();
  EXPECT_EQ(ref.referent(), *array);
}

TEST_F(ScavengerTest, PreserveWeakReferenceImmediateReferent) {
  HandleScope scope(thread_);
  Int obj(&scope, SmallInt::fromWord(1234));
  Object none(&scope, NoneType::object());
  WeakRef ref(&scope, runtime_.newWeakRef(thread_, obj, none));
  runtime_.collectGarbage();
  EXPECT_EQ(ref.referent(), SmallInt::fromWord(1234));
}

TEST_F(ScavengerTest, ClearWeakReference) {
  HandleScope scope;
  Object none(&scope, NoneType::object());
  Object ref(&scope, *none);
  {
    Tuple array(&scope, runtime_.newTuple(10));
    WeakRef ref_inner(&scope,
                      runtime_.newWeakRef(Thread::current(), array, none));
    ref = *ref_inner;
    runtime_.collectGarbage();
    EXPECT_EQ(ref_inner.referent(), *array);
  }
  runtime_.collectGarbage();
  EXPECT_EQ(WeakRef::cast(*ref).referent(), NoneType::object());
}

TEST_F(ScavengerTest, ClearWeakLinkReferences) {
  HandleScope scope(thread_);
  Object none(&scope, NoneType::object());
  Object link0(&scope, *none);
  Object link1(&scope, *none);
  Object link2(&scope, *none);
  Tuple referent1(&scope, runtime_.newTuple(11));
  {
    Tuple referent0(&scope, runtime_.newTuple(10));
    Tuple referent2(&scope, runtime_.newTuple(11));
    WeakLink link0_inner(&scope,
                         runtime_.newWeakLink(thread_, referent0, none, none));
    WeakLink link1_inner(
        &scope, runtime_.newWeakLink(thread_, referent1, link0_inner, none));
    WeakLink link2_inner(
        &scope, runtime_.newWeakLink(thread_, referent2, link1_inner, none));
    link0_inner.setNext(*link1_inner);
    link1_inner.setNext(*link2_inner);

    link0 = *link0_inner;
    link1 = *link1_inner;
    link2 = *link2_inner;

    runtime_.collectGarbage();
    EXPECT_EQ(link0_inner.referent(), *referent0);
    EXPECT_EQ(link1_inner.referent(), *referent1);
    EXPECT_EQ(link2_inner.referent(), *referent2);
  }
  runtime_.collectGarbage();
  EXPECT_EQ(WeakRef::cast(*link0).referent(), NoneType::object());
  EXPECT_EQ(WeakRef::cast(*link1).referent(), *referent1);
  EXPECT_EQ(WeakRef::cast(*link2).referent(), NoneType::object());
}

TEST_F(ScavengerTest, PreserveSomeClearSomeReferents) {
  HandleScope scope(thread_);

  // Create strongly referenced heap allocated objects.
  Tuple strongrefs(&scope, runtime_.newTuple(4));
  for (word i = 0; i < strongrefs.length(); i++) {
    Float elt(&scope, runtime_.newFloat(i));
    strongrefs.atPut(i, *elt);
  }

  // Create a parallel array of weak references with the strongly referenced
  // objects as referents.
  Tuple weakrefs(&scope, runtime_.newTuple(4));
  for (word i = 0; i < weakrefs.length(); i++) {
    Object obj(&scope, strongrefs.at(i));
    Object none(&scope, NoneType::object());
    WeakRef elt(&scope, runtime_.newWeakRef(thread_, obj, none));
    weakrefs.atPut(i, *elt);
  }

  // Make sure the weak references point to the expected referents.
  EXPECT_EQ(strongrefs.at(0), WeakRef::cast(weakrefs.at(0)).referent());
  EXPECT_EQ(strongrefs.at(1), WeakRef::cast(weakrefs.at(1)).referent());
  EXPECT_EQ(strongrefs.at(2), WeakRef::cast(weakrefs.at(2)).referent());
  EXPECT_EQ(strongrefs.at(3), WeakRef::cast(weakrefs.at(3)).referent());

  // Now do a garbage collection.
  runtime_.collectGarbage();

  // Make sure that the weak references still point to the expected referents.
  EXPECT_EQ(strongrefs.at(0), WeakRef::cast(weakrefs.at(0)).referent());
  EXPECT_EQ(strongrefs.at(1), WeakRef::cast(weakrefs.at(1)).referent());
  EXPECT_EQ(strongrefs.at(2), WeakRef::cast(weakrefs.at(2)).referent());
  EXPECT_EQ(strongrefs.at(3), WeakRef::cast(weakrefs.at(3)).referent());

  // Clear the odd indexed strong references.
  strongrefs.atPut(1, NoneType::object());
  strongrefs.atPut(3, NoneType::object());
  runtime_.collectGarbage();

  // Now do another garbage collection.  This one should clear just the weak
  // references that point to objects that are no longer strongly referenced.

  // Check that the strongly referenced referents are preserved and the weakly
  // referenced referents are now cleared.
  EXPECT_EQ(strongrefs.at(0), WeakRef::cast(weakrefs.at(0)).referent());
  EXPECT_EQ(NoneType::object(), WeakRef::cast(weakrefs.at(1)).referent());
  EXPECT_EQ(strongrefs.at(2), WeakRef::cast(weakrefs.at(2)).referent());
  EXPECT_EQ(NoneType::object(), WeakRef::cast(weakrefs.at(3)).referent());

  // Clear the even indexed strong references.
  strongrefs.atPut(0, NoneType::object());
  strongrefs.atPut(2, NoneType::object());

  // Do a final garbage collection.  There are no more strongly referenced
  // objects so all of the weak references should be cleared.
  runtime_.collectGarbage();

  // Check that all of the referents are cleared.
  EXPECT_EQ(NoneType::object(), WeakRef::cast(weakrefs.at(0)).referent());
  EXPECT_EQ(NoneType::object(), WeakRef::cast(weakrefs.at(1)).referent());
  EXPECT_EQ(NoneType::object(), WeakRef::cast(weakrefs.at(2)).referent());
  EXPECT_EQ(NoneType::object(), WeakRef::cast(weakrefs.at(3)).referent());
}

TEST_F(ScavengerTest, BaseCallback) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
a = 1
b = 2
def f(ref):
  global a
  a = 3
def g(ref, c=4):
  global b
  b = c
)")
                   .isError());
  Module main(&scope, findModule(&runtime_, "__main__"));
  Object none(&scope, NoneType::object());
  Object ref1(&scope, *none);
  Object ref2(&scope, *none);
  {
    Tuple array1(&scope, runtime_.newTuple(10));
    Function func_f(&scope, moduleAt(&runtime_, main, "f"));
    WeakRef ref1_inner(&scope, runtime_.newWeakRef(thread_, array1, func_f));
    ref1 = *ref1_inner;

    Tuple array2(&scope, runtime_.newTuple(10));
    Function func_g(&scope, moduleAt(&runtime_, main, "g"));
    WeakRef ref2_inner(&scope, runtime_.newWeakRef(thread_, array2, func_g));
    ref2 = *ref2_inner;

    runtime_.collectGarbage();

    EXPECT_EQ(ref1_inner.referent(), *array1);
    EXPECT_EQ(ref2_inner.referent(), *array2);
    SmallInt a(&scope, moduleAt(&runtime_, main, "a"));
    SmallInt b(&scope, moduleAt(&runtime_, main, "b"));
    EXPECT_EQ(a.value(), 1);
    EXPECT_EQ(b.value(), 2);
  }
  runtime_.collectGarbage();

  EXPECT_EQ(WeakRef::cast(*ref1).referent(), NoneType::object());
  EXPECT_EQ(WeakRef::cast(*ref1).callback(), NoneType::object());
  EXPECT_EQ(WeakRef::cast(*ref2).referent(), NoneType::object());
  EXPECT_EQ(WeakRef::cast(*ref2).callback(), NoneType::object());
  SmallInt a(&scope, moduleAt(&runtime_, main, "a"));
  SmallInt b(&scope, moduleAt(&runtime_, main, "b"));
  EXPECT_EQ(a.value(), 3);
  EXPECT_EQ(b.value(), 4);
}

TEST_F(ScavengerTest, MixCallback) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
a = 1
b = 2
def f(ref):
  global a
  a = 3
def g(ref, c=4):
  global b
  b = c
)")
                   .isError());
  Module main(&scope, findModule(&runtime_, "__main__"));

  Tuple array1(&scope, runtime_.newTuple(10));
  Function func_f(&scope, moduleAt(&runtime_, main, "f"));
  WeakRef ref1(&scope, runtime_.newWeakRef(thread_, array1, func_f));
  Object ref2(&scope, NoneType::object());
  {
    Tuple array2(&scope, runtime_.newTuple(10));
    Function func_g(&scope, moduleAt(&runtime_, main, "g"));
    WeakRef ref2_inner(&scope, runtime_.newWeakRef(thread_, array2, func_g));
    ref2 = *ref2_inner;

    runtime_.collectGarbage();

    EXPECT_EQ(ref1.referent(), *array1);
    EXPECT_EQ(ref2_inner.referent(), *array2);
    SmallInt a(&scope, moduleAt(&runtime_, main, "a"));
    SmallInt b(&scope, moduleAt(&runtime_, main, "b"));
    EXPECT_EQ(a.value(), 1);
    EXPECT_EQ(b.value(), 2);
  }
  runtime_.collectGarbage();

  EXPECT_EQ(ref1.referent(), *array1);
  EXPECT_EQ(ref1.callback(), *func_f);
  EXPECT_EQ(WeakRef::cast(*ref2).referent(), NoneType::object());
  EXPECT_EQ(WeakRef::cast(*ref2).callback(), NoneType::object());
  SmallInt a(&scope, moduleAt(&runtime_, main, "a"));
  SmallInt b(&scope, moduleAt(&runtime_, main, "b"));
  EXPECT_EQ(a.value(), 1);
  EXPECT_EQ(b.value(), 4);
}

static RawObject doGarbageCollection(Thread* thread, Frame*, word) {
  thread->runtime()->collectGarbage();
  return NoneType::object();
}

TEST_F(ScavengerTest, CallbackInvokeGC) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
a = 1
def g(ref, b=2):
  global a
  a = b
)")
                   .isError());
  Module main(&scope, findModule(&runtime_, "__main__"));
  Object ref1(&scope, NoneType::object());
  Object ref2(&scope, NoneType::object());
  {
    Tuple array1(&scope, runtime_.newTuple(10));
    Str name(&scope, runtime_.newStrFromCStr("collect"));
    Object empty_tuple(&scope, runtime_.emptyTuple());
    Code code(&scope,
              runtime_.newBuiltinCode(/*argcount=*/0, /*posonlyargcount=*/0,
                                      /*kwonlyargcount=*/0, /*flags=*/0,
                                      doGarbageCollection,
                                      /*parameter_names=*/empty_tuple, name));
    Dict globals(&scope, runtime_.newDict());
    Function collect(
        &scope, runtime_.newFunctionWithCode(thread_, name, code, globals));

    WeakRef ref1_inner(&scope, runtime_.newWeakRef(thread_, array1, collect));
    ref1 = *ref1_inner;

    Tuple array2(&scope, runtime_.newTuple(10));
    Function func_g(&scope, moduleAt(&runtime_, main, "g"));
    WeakRef ref2_inner(&scope, runtime_.newWeakRef(thread_, array2, func_g));
    ref2 = *ref2_inner;

    runtime_.collectGarbage();

    EXPECT_EQ(ref1_inner.referent(), *array1);
    EXPECT_EQ(ref2_inner.referent(), *array2);
    SmallInt a(&scope, moduleAt(&runtime_, main, "a"));
    EXPECT_EQ(a.value(), 1);
  }
  runtime_.collectGarbage();

  EXPECT_EQ(WeakRef::cast(*ref1).referent(), NoneType::object());
  EXPECT_EQ(WeakRef::cast(*ref1).callback(), NoneType::object());
  EXPECT_EQ(WeakRef::cast(*ref2).referent(), NoneType::object());
  EXPECT_EQ(WeakRef::cast(*ref2).callback(), NoneType::object());
  SmallInt a(&scope, moduleAt(&runtime_, main, "a"));
  EXPECT_EQ(a.value(), 2);
}

TEST_F(ScavengerTest, IgnoreCallbackException) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
a = 1
b = 2
callback_ran = False
callback_returned = False
def f(ref):
  global callback_ran
  callback_ran = True
  raise AttributeError("aloha")
  global callback_returned
  callback_returned = True
def g(ref, c=4):
  global b
  b = c
)")
                   .isError());
  Module main(&scope, findModule(&runtime_, "__main__"));
  Object ref1(&scope, NoneType::object());
  Object ref2(&scope, NoneType::object());
  {
    Tuple array1(&scope, runtime_.newTuple(10));
    Function func_f(&scope, moduleAt(&runtime_, main, "f"));
    WeakRef ref1_inner(&scope, runtime_.newWeakRef(thread_, array1, func_f));
    ref1 = *ref1_inner;

    Tuple array2(&scope, runtime_.newTuple(10));
    Function func_g(&scope, moduleAt(&runtime_, main, "g"));
    WeakRef ref2_inner(&scope, runtime_.newWeakRef(thread_, array2, func_g));
    ref2 = *ref2_inner;

    runtime_.collectGarbage();

    EXPECT_EQ(ref1_inner.referent(), *array1);
    EXPECT_EQ(ref2_inner.referent(), *array2);
    SmallInt a(&scope, moduleAt(&runtime_, main, "a"));
    SmallInt b(&scope, moduleAt(&runtime_, main, "b"));
    EXPECT_EQ(a.value(), 1);
    EXPECT_EQ(b.value(), 2);
  }

  runtime_.collectGarbage();
  EXPECT_FALSE(Thread::current()->hasPendingException());
  EXPECT_EQ(moduleAt(&runtime_, main, "callback_ran"), Bool::trueObj());
  EXPECT_EQ(moduleAt(&runtime_, main, "callback_returned"), Bool::falseObj());

  EXPECT_EQ(WeakRef::cast(*ref1).referent(), NoneType::object());
  EXPECT_EQ(WeakRef::cast(*ref1).callback(), NoneType::object());
  EXPECT_EQ(WeakRef::cast(*ref2).referent(), NoneType::object());
  EXPECT_EQ(WeakRef::cast(*ref2).callback(), NoneType::object());
  SmallInt a(&scope, moduleAt(&runtime_, main, "a"));
  SmallInt b(&scope, moduleAt(&runtime_, main, "b"));
  EXPECT_EQ(a.value(), 1);
  EXPECT_EQ(b.value(), 4);
}

}  // namespace python

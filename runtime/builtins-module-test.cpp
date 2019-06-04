#include "gtest/gtest.h"

#include "builtins-module.h"
#include "runtime.h"
#include "test-utils.h"
#include "trampolines.h"

namespace python {

using namespace testing;

TEST(BuiltinsModuleTest, BuiltinCallableOnTypeReturnsTrue) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class Foo:
  pass

a = callable(Foo)
  )")
                   .isError());
  HandleScope scope;
  Module main(&scope, findModule(&runtime, "__main__"));
  Bool a(&scope, moduleAt(&runtime, main, "a"));
  EXPECT_TRUE(a.value());
}

TEST(BuiltinsModuleTest, BuiltinCallableOnMethodReturnsTrue) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class Foo:
  def bar():
    return None

a = callable(Foo.bar)
b = callable(Foo().bar)
  )")
                   .isError());
  HandleScope scope;
  Module main(&scope, findModule(&runtime, "__main__"));
  Bool a(&scope, moduleAt(&runtime, main, "a"));
  Bool b(&scope, moduleAt(&runtime, main, "b"));
  EXPECT_TRUE(a.value());
  EXPECT_TRUE(b.value());
}

TEST(BuiltinsModuleTest, BuiltinCallableOnNonCallableReturnsFalse) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
a = callable(1)
b = callable("hello")
  )")
                   .isError());
  HandleScope scope;
  Module main(&scope, findModule(&runtime, "__main__"));
  Bool a(&scope, moduleAt(&runtime, main, "a"));
  Bool b(&scope, moduleAt(&runtime, main, "b"));
  EXPECT_FALSE(a.value());
  EXPECT_FALSE(b.value());
}

TEST(BuiltinsModuleTest, BuiltinCallableOnObjectWithCallOnTypeReturnsTrue) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class Foo:
  def __call__(self):
    pass

f = Foo()
a = callable(f)
  )")
                   .isError());
  HandleScope scope;
  Module main(&scope, findModule(&runtime, "__main__"));
  Bool a(&scope, moduleAt(&runtime, main, "a"));
  EXPECT_TRUE(a.value());
}

TEST(BuiltinsModuleTest,
     BuiltinCallableOnObjectWithInstanceCallButNoTypeCallReturnsFalse) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class Foo:
  pass

def fakecall():
  pass

f = Foo()
f.__call__ = fakecall
a = callable(f)
  )")
                   .isError());
  HandleScope scope;
  Module main(&scope, findModule(&runtime, "__main__"));
  Bool a(&scope, moduleAt(&runtime, main, "a"));
  EXPECT_FALSE(a.value());
}

TEST(BuiltinsModuleTest, BuiltinChrWithNonIntRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  Object i(&scope, NoneType::object());
  EXPECT_TRUE(raisedWithStr(runBuiltin(BuiltinsModule::chr, i),
                            LayoutId::kTypeError,
                            "an integer is required (got type NoneType)"));
}

TEST(BuiltinsModuleTest, BuiltinChrWithLargeIntRaisesOverflowError) {
  Runtime runtime;
  HandleScope scope;
  Object i(&scope, runtime.newInt(kMaxWord));
  EXPECT_TRUE(raisedWithStr(runBuiltin(BuiltinsModule::chr, i),
                            LayoutId::kOverflowError,
                            "Python int too large to convert to C long"));
}

TEST(BuiltinsModuleTest, BuiltinChrWithNegativeIntRaisesValueError) {
  Runtime runtime;
  HandleScope scope;
  Object i(&scope, SmallInt::fromWord(-1));
  EXPECT_TRUE(raisedWithStr(runBuiltin(BuiltinsModule::chr, i),
                            LayoutId::kValueError,
                            "chr() arg not in range(0x110000)"));
}

TEST(BuiltinsModuleTest, BuiltinChrWithNonUnicodeIntRaisesValueError) {
  Runtime runtime;
  HandleScope scope;
  Object i(&scope, SmallInt::fromWord(kMaxUnicode + 1));
  EXPECT_TRUE(raisedWithStr(runBuiltin(BuiltinsModule::chr, i),
                            LayoutId::kValueError,
                            "chr() arg not in range(0x110000)"));
}

TEST(BuiltinsModuleTest, BuiltinChrWithMaxUnicodeReturnsString) {
  Runtime runtime;
  HandleScope scope;
  Object i(&scope, SmallInt::fromWord(kMaxUnicode));
  EXPECT_EQ(runBuiltin(BuiltinsModule::chr, i),
            SmallStr::fromCodePoint(kMaxUnicode));
}

TEST(BuiltinsModuleTest, BuiltinChrWithASCIIReturnsString) {
  Runtime runtime;
  HandleScope scope;
  Object i(&scope, SmallInt::fromWord('*'));
  EXPECT_EQ(runBuiltin(BuiltinsModule::chr, i), SmallStr::fromCStr("*"));
}

TEST(BuiltinsModuleTest, BuiltinChrWithIntSubclassReturnsString) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class C(int): pass
i = C(42)
)")
                   .isError());
  Object i(&scope, moduleAt(&runtime, "__main__", "i"));
  EXPECT_EQ(runBuiltin(BuiltinsModule::chr, i), SmallStr::fromCStr("*"));
}

TEST(BuiltinsModuleTest, DirCallsDunderDirReturnsSortedList) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class C:
  def __dir__(self):
    return ["B", "A"]
c = C()
d = dir(c)
)")
                   .isError());
  Object d_obj(&scope, moduleAt(&runtime, "__main__", "d"));
  ASSERT_TRUE(d_obj.isList());
  List d(&scope, *d_obj);
  ASSERT_EQ(d.numItems(), 2);
  EXPECT_TRUE(isStrEqualsCStr(d.at(0), "A"));
  EXPECT_TRUE(isStrEqualsCStr(d.at(1), "B"));
}

TEST(BuiltinsModuleDeathTest, DirWithoutObjectCallsLocals) {
  Runtime runtime;
  // locals() is not implemented yet, so we will die here.
  ASSERT_DEATH(static_cast<void>(runFromCStr(&runtime, R"(
dir()
)")),
               "locals()");
}

TEST(BuiltinsModuleTest, EllipsisMatchesEllipsis) {
  Runtime runtime;
  EXPECT_EQ(moduleAt(&runtime, "builtins", "Ellipsis"), runtime.ellipsis());
}

TEST(BuiltinsModuleTest, IdReturnsInt) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object obj(&scope, runtime.newInt(12345));
  EXPECT_TRUE(runBuiltin(BuiltinsModule::id, obj).isInt());
}

TEST(BuiltinsModuleTest, IdDoesNotChangeAfterGC) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object obj(&scope, runtime.newStrFromCStr("hello world foobar"));
  Object id_before(&scope, runBuiltin(BuiltinsModule::id, obj));
  runtime.collectGarbage();
  Object id_after(&scope, runBuiltin(BuiltinsModule::id, obj));
  EXPECT_EQ(*id_before, *id_after);
}

TEST(BuiltinsModuleTest, IdReturnsDifferentValueForDifferentObject) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object obj1(&scope, runtime.newStrFromCStr("hello world foobar"));
  Object obj2(&scope, runtime.newStrFromCStr("hello world foobarbaz"));
  EXPECT_NE(runBuiltin(BuiltinsModule::id, obj1),
            runBuiltin(BuiltinsModule::id, obj2));
}

TEST(BuiltinsModuleTest, BuiltinLen) {
  Runtime runtime;
  std::string result = compileAndRunToString(&runtime, "print(len([1,2,3]))");
  EXPECT_EQ(result, "3\n");
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, "print(len(1))"),
                            LayoutId::kTypeError, "object has no len()"));
}

TEST(ThreadTest, BuiltinLenGetLenFromDict) {
  Runtime runtime;

  ASSERT_FALSE(runFromCStr(&runtime, R"(
len0 = len({})
len1 = len({'one': 1})
len5 = len({'one': 1, 'two': 2, 'three': 3, 'four': 4, 'five': 5})
)")
                   .isError());

  HandleScope scope;
  Module main(&scope, findModule(&runtime, "__main__"));
  Object len0(&scope, moduleAt(&runtime, main, "len0"));
  EXPECT_EQ(*len0, SmallInt::fromWord(0));
  Object len1(&scope, moduleAt(&runtime, main, "len1"));
  EXPECT_EQ(*len1, SmallInt::fromWord(1));
  Object len5(&scope, moduleAt(&runtime, main, "len5"));
  EXPECT_EQ(*len5, SmallInt::fromWord(5));
}

TEST(ThreadTest, BuiltinLenGetLenFromList) {
  Runtime runtime;

  ASSERT_FALSE(runFromCStr(&runtime, R"(
len0 = len([])
len1 = len([1])
len5 = len([1,2,3,4,5])
)")
                   .isError());

  HandleScope scope;
  Module main(&scope, findModule(&runtime, "__main__"));
  Object len0(&scope, moduleAt(&runtime, main, "len0"));
  EXPECT_EQ(*len0, SmallInt::fromWord(0));
  Object len1(&scope, moduleAt(&runtime, main, "len1"));
  EXPECT_EQ(*len1, SmallInt::fromWord(1));
  Object len5(&scope, moduleAt(&runtime, main, "len5"));
  EXPECT_EQ(*len5, SmallInt::fromWord(5));
}

TEST(ThreadTest, BuiltinLenGetLenFromSet) {
  Runtime runtime;

  ASSERT_FALSE(runFromCStr(&runtime, R"(
len1 = len({1})
len5 = len({1,2,3,4,5})
)")
                   .isError());

  HandleScope scope;
  Module main(&scope, findModule(&runtime, "__main__"));
  // TODO(cshapiro): test the empty set when we have builtins.set defined.
  Object len1(&scope, moduleAt(&runtime, main, "len1"));
  EXPECT_EQ(*len1, SmallInt::fromWord(1));
  Object len5(&scope, moduleAt(&runtime, main, "len5"));
  EXPECT_EQ(*len5, SmallInt::fromWord(5));
}

TEST(BuiltinsModuleTest, BuiltinOrd) {
  Runtime runtime;
  std::string result = compileAndRunToString(&runtime, "print(ord('A'))");
  EXPECT_EQ(result, "65\n");
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, "print(ord(1))"),
                            LayoutId::kTypeError,
                            "Unsupported type in builtin 'ord'"));
}

TEST(BuiltinsModuleTest, BuiltinOrdSupportNonASCII) {
  Runtime runtime;
  HandleScope scope;
  Str two_bytes(&scope, runtime.newStrFromCStr("\xC3\xA9"));
  Object two_ord(&scope, runBuiltin(BuiltinsModule::ord, two_bytes));
  EXPECT_TRUE(isIntEqualsWord(*two_ord, 0xE9));

  Str three_bytes(&scope, runtime.newStrFromCStr("\xE2\xB3\x80"));
  Object three_ord(&scope, runBuiltin(BuiltinsModule::ord, three_bytes));
  EXPECT_TRUE(isIntEqualsWord(*three_ord, 0x2CC0));

  Str four_bytes(&scope, runtime.newStrFromCStr("\xF0\x9F\x86\x92"));
  Object four_ord(&scope, runBuiltin(BuiltinsModule::ord, four_bytes));
  EXPECT_TRUE(isIntEqualsWord(*four_ord, 0x1F192));
}

TEST(BuiltinsModuleTest, BuiltinOrdWithMultiCodepointStringRaisesError) {
  Runtime runtime;
  HandleScope scope;
  Str two_chars(&scope, runtime.newStrFromCStr("ab"));
  EXPECT_TRUE(raisedWithStr(runBuiltin(BuiltinsModule::ord, two_chars),
                            LayoutId::kTypeError,
                            "Builtin 'ord' expects string of length 1"));
}

TEST(BuiltinsModuleTest, BuiltInPrintStdOut) {
  const char* src = R"(
import sys
print("hello", file=sys.stdout)
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "hello\n");
}

TEST(BuiltinsModuleTest, BuiltInPrintEnd) {
  const char* src = R"(
import sys
print("hi", end='ho')
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "hiho");
}

TEST(BuiltinsModuleTest, BuiltInPrintStdOutEnd) {
  const char* src = R"(
import sys
print("hi", end='ho', file=sys.stdout)
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "hiho");
}

TEST(BuiltinsModuleTest, BuiltInPrintStdErr) {
  const char* src = R"(
import sys
print("hi", file=sys.stderr, end='ya')
)";
  Runtime runtime;
  std::string output = compileAndRunToStderrString(&runtime, src);
  EXPECT_EQ(output, "hiya");
}

TEST(BuiltinsModuleTest, BuiltInPrintNone) {
  const char* src = R"(
print(None)
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "None\n");
}

TEST(BuiltinsModuleTest, BuiltInPrintStrList) {
  const char* src = R"(
print(['one', 'two'])
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "['one', 'two']\n");
}

TEST(BuiltinsModuleTest, PrintWithFileNoneWritesToSysStdout) {
  Runtime runtime;
  EXPECT_EQ(compileAndRunToString(&runtime, "print(7, file=None)"), "7\n");
}

TEST(BuiltinsModuleTest, PrintWithSysStdoutNoneDoesNothing) {
  Runtime runtime;
  EXPECT_EQ(compileAndRunToString(&runtime, R"(
import sys
sys.stdout = None
print("hello")
)"),
            "");
}

TEST(BuiltinsModuleTest, PrintWithoutSysStdoutRaisesRuntimeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
import sys
del sys.stdout
print()
)"),
                            LayoutId::kRuntimeError, "lost sys.stdout"));
}

TEST(BuiltinsModuleTest, PrintWithFlushCallsFileFlush) {
  Runtime runtime;
  EXPECT_TRUE(raised(runFromCStr(&runtime, R"(
import sys
class C:
  def write(self, text): pass
  def flush(self): raise UserWarning()
sys.stdout = C()
print(flush=True)
)"),
                     LayoutId::kUserWarning));
}

TEST(BuiltinsModuleTest, BuiltInReprOnUserTypeWithDunderRepr) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class Foo:
  def __repr__(self):
    return "foo"

a = repr(Foo())
)")
                   .isError());
  HandleScope scope;
  Module main(&scope, findModule(&runtime, "__main__"));
  Object a(&scope, moduleAt(&runtime, main, "a"));
  EXPECT_TRUE(isStrEqualsCStr(*a, "foo"));
}

TEST(BuiltinsModuleTest, BuiltInReprOnClass) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, "result = repr(int)").isError());
  HandleScope scope;
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_TRUE(isStrEqualsCStr(*result, "<class 'int'>"));
}

TEST(BuiltinsModuleTest, BuiltInAsciiCallsDunderRepr) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class Foo:
  def __repr__(self):
    return "foo"

a = ascii(Foo())
)")
                   .isError());
  HandleScope scope;
  Module main(&scope, findModule(&runtime, "__main__"));
  Object a(&scope, moduleAt(&runtime, main, "a"));
  EXPECT_TRUE(isStrEqualsCStr(*a, "foo"));
}

TEST(BuiltinsModuleTest, DunderBuildClassWithNonFunctionRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  Object body(&scope, NoneType::object());
  Object name(&scope, runtime.newStrFromCStr("a"));
  Object metaclass(&scope, Unbound::object());
  Object bootstrap(&scope, Bool::falseObj());
  Object bases(&scope, runtime.newTuple(0));
  Object kwargs(&scope, runtime.newDict());
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(BuiltinsModule::dunderBuildClass, body, name, metaclass,
                 bootstrap, bases, kwargs),
      LayoutId::kTypeError, "__build_class__: func must be a function"));
}

TEST(BuiltinsModuleTest, DunderBuildClassWithNonStringRaisesTypeError) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, "def f(): pass").isError());
  Object body(&scope, moduleAt(&runtime, "__main__", "f"));
  Object name(&scope, NoneType::object());
  Object metaclass(&scope, Unbound::object());
  Object bootstrap(&scope, Bool::falseObj());
  Object bases(&scope, runtime.newTuple(0));
  Object kwargs(&scope, runtime.newDict());
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(BuiltinsModule::dunderBuildClass, body, name, metaclass,
                 bootstrap, bases, kwargs),
      LayoutId::kTypeError, "__build_class__: name is not a string"));
}

TEST(BuiltinsModuleTest, DunderBuildClassCallsMetaclass) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class Meta(type):
  def __new__(mcls, name, bases, namespace, *args, **kwargs):
    return (mcls, name, bases, namespace, args, kwargs)
class C(int, float, metaclass=Meta, hello="world"):
  x = 42
)")
                   .isError());
  Object meta(&scope, moduleAt(&runtime, "__main__", "Meta"));
  Object c_obj(&scope, moduleAt(&runtime, "__main__", "C"));
  ASSERT_TRUE(c_obj.isTuple());
  Tuple c(&scope, *c_obj);
  ASSERT_EQ(c.length(), 6);
  EXPECT_EQ(c.at(0), meta);
  EXPECT_TRUE(isStrEqualsCStr(c.at(1), "C"));

  ASSERT_TRUE(c.at(2).isTuple());
  Tuple c_bases(&scope, c.at(2));
  ASSERT_EQ(c_bases.length(), 2);
  EXPECT_EQ(c_bases.at(0), runtime.typeAt(LayoutId::kInt));
  EXPECT_EQ(c_bases.at(1), runtime.typeAt(LayoutId::kFloat));

  ASSERT_TRUE(c.at(3).isDict());
  Dict c_namespace(&scope, c.at(3));
  Object x(&scope, runtime.newStrFromCStr("x"));
  EXPECT_TRUE(runtime.dictIncludes(thread, c_namespace, x));
  ASSERT_TRUE(c.at(4).isTuple());
  EXPECT_EQ(Tuple::cast(c.at(4)).length(), 0);
  Object hello(&scope, runtime.newStrFromCStr("hello"));
  ASSERT_TRUE(c.at(5).isDict());
  Dict c_kwargs(&scope, c.at(5));
  EXPECT_EQ(c_kwargs.numItems(), 1);
  EXPECT_TRUE(
      isStrEqualsCStr(runtime.dictAt(thread, c_kwargs, hello), "world"));
}

TEST(BuiltinsModuleTest, DunderBuildClassCalculatesMostSpecificMetaclass) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class Meta(type): pass
class C1(int, metaclass=Meta): pass
class C2(C1, metaclass=type): pass
t1 = type(C1)
t2 = type(C2)
)")
                   .isError());
  Object meta(&scope, moduleAt(&runtime, "__main__", "Meta"));
  Object t1(&scope, moduleAt(&runtime, "__main__", "t1"));
  Object t2(&scope, moduleAt(&runtime, "__main__", "t2"));
  ASSERT_TRUE(t1.isType());
  ASSERT_TRUE(t2.isType());
  EXPECT_EQ(t1, meta);
  EXPECT_EQ(t2, meta);
}

TEST(BuiltinsModuleTest,
     DunderBuildClassWithIncompatibleMetaclassesRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, R"(
class M1(type): pass
class M2(type): pass
class C1(metaclass=M1): pass
class C2(C1, metaclass=M2): pass
)"),
      LayoutId::kTypeError,
      "metaclass conflict: the metaclass of a derived class must be a "
      "(non-strict) subclass of the metaclasses of all its bases"));
}

TEST(BuiltinsModuleTest, DunderBuildClassWithMeetMetaclassUsesMeet) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class M1(type): pass
class M2(type): pass
class M3(M1, M2): pass
class C1(metaclass=M1): pass
class C2(metaclass=M2): pass
class C3(C1, C2, metaclass=M3): pass
t1 = type(C1)
t2 = type(C2)
t3 = type(C3)
)")
                   .isError());
  Object m1(&scope, moduleAt(&runtime, "__main__", "M1"));
  Object m2(&scope, moduleAt(&runtime, "__main__", "M2"));
  Object m3(&scope, moduleAt(&runtime, "__main__", "M3"));
  Object t1(&scope, moduleAt(&runtime, "__main__", "t1"));
  Object t2(&scope, moduleAt(&runtime, "__main__", "t2"));
  Object t3(&scope, moduleAt(&runtime, "__main__", "t3"));
  ASSERT_TRUE(t1.isType());
  ASSERT_TRUE(t2.isType());
  ASSERT_TRUE(t3.isType());
  EXPECT_EQ(t1, m1);
  EXPECT_EQ(t2, m2);
  EXPECT_EQ(t3, m3);
}

TEST(BuiltinsModuleTest, DunderBuildClassPropagatesDunderPrepareError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
class Meta(type):
  @classmethod
  def __prepare__(cls, *args, **kwds):
    raise IndentationError("foo")
class C(metaclass=Meta):
  pass
)"),
                            LayoutId::kIndentationError, "foo"));
}

TEST(BuiltinsModuleTest, DunderBuildClassWithNonDictPrepareRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(
      raisedWithStr(runFromCStr(&runtime, R"(
class Meta(type):
  @classmethod
  def __prepare__(cls, *args, **kwds):
    return 42
class C(metaclass=Meta):
  pass
)"),
                    LayoutId::kTypeError,
                    "Meta.__prepare__() must return a mapping, not int"));
}

TEST(BuiltinsModuleTest,
     DunderBuildClassWithNonTypeMetaclassAndNonDictPrepareRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, R"(
class Meta:
  def __prepare__(self, *args, **kwds):
    return 42
class C(metaclass=Meta()):
  pass
)"),
      LayoutId::kTypeError,
      "<metaclass>.__prepare__() must return a mapping, not int"));
}

TEST(BuiltinsModuleTest, DunderBuildClassUsesDunderPrepareForClassDict) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class Meta(type):
  @classmethod
  def __prepare__(cls, *args, **kwds):
    return {"foo": 42}
class C(metaclass=Meta):
  pass
result = C.foo
)")
                   .isError());
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_TRUE(isIntEqualsWord(*result, 42));
}

TEST(BuiltinsModuleTest, DunderBuildClassPassesNameBasesAndKwargsToPrepare) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class Meta(type):
  def __init__(metacls, name, bases, namespace, **kwargs):
    pass
  def __new__(metacls, name, bases, namespace, **kwargs):
    return super().__new__(metacls, name, bases, namespace)
  @classmethod
  def __prepare__(metacls, name, bases, **kwargs):
    return {"foo": name, "bar": bases[0], "baz": kwargs["answer"]}
class C(int, metaclass=Meta, answer=42):
  pass
name = C.foo
base = C.bar
answer = C.baz
)")
                   .isError());
  Object name(&scope, moduleAt(&runtime, "__main__", "name"));
  Object base(&scope, moduleAt(&runtime, "__main__", "base"));
  Object answer(&scope, moduleAt(&runtime, "__main__", "answer"));
  EXPECT_TRUE(isStrEqualsCStr(*name, "C"));
  EXPECT_EQ(base, runtime.typeAt(LayoutId::kInt));
  EXPECT_TRUE(isIntEqualsWord(*answer, 42));
}

TEST(BuiltinsModuleTest, DunderBuildClassWithRaisingBodyPropagatesException) {
  Runtime runtime;
  EXPECT_TRUE(raised(runFromCStr(&runtime, R"(
class C:
  raise UserWarning()
)"),
                     LayoutId::kUserWarning));
}

TEST(BuiltinsModuleTest, GetAttrFromClassReturnsValue) {
  const char* src = R"(
class Foo:
  bar = 1
obj = getattr(Foo, 'bar')
)";
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, src).isError());
  Module main(&scope, findModule(&runtime, "__main__"));
  Object obj(&scope, moduleAt(&runtime, main, "obj"));
  EXPECT_EQ(*obj, SmallInt::fromWord(1));
}

TEST(BuiltinsModuleTest, GetAttrFromInstanceReturnsValue) {
  const char* src = R"(
class Foo:
  bar = 1
obj = getattr(Foo(), 'bar')
)";
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, src).isError());
  Module main(&scope, findModule(&runtime, "__main__"));
  Object obj(&scope, moduleAt(&runtime, main, "obj"));
  EXPECT_EQ(*obj, SmallInt::fromWord(1));
}

TEST(BuiltinsModuleTest, GetAttrFromInstanceWithMissingAttrReturnsDefault) {
  const char* src = R"(
class Foo: pass
obj = getattr(Foo(), 'bar', 2)
)";
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, src).isError());
  Module main(&scope, findModule(&runtime, "__main__"));
  Object obj(&scope, moduleAt(&runtime, main, "obj"));
  EXPECT_EQ(*obj, SmallInt::fromWord(2));
}

TEST(BuiltinsModuleTest, GetAttrWithNonStringAttrRaisesTypeError) {
  const char* src = R"(
class Foo: pass
getattr(Foo(), 1)
)";
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, src), LayoutId::kTypeError,
                            "getattr(): attribute name must be string"));
}

TEST(BuiltinsModuleTest, GetAttrWithNonStringAttrAndDefaultRaisesTypeError) {
  const char* src = R"(
class Foo: pass
getattr(Foo(), 1, 2)
)";
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, src), LayoutId::kTypeError,
                            "getattr(): attribute name must be string"));
}

TEST(BuiltinsModuleTest,
     GetAttrFromClassMissingAttrWithoutDefaultRaisesAttributeError) {
  const char* src = R"(
class Foo:
  bar = 1
getattr(Foo, 'foo')
)";
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, src),
                            LayoutId::kAttributeError,
                            "type object 'Foo' has no attribute 'foo'"));
}

TEST(BuiltinsModuleTest, HasAttrFromClassMissingAttrReturnsFalse) {
  const char* src = R"(
class Foo: pass
obj = hasattr(Foo, 'bar')
)";
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, src).isError());
  Module main(&scope, findModule(&runtime, "__main__"));
  Object obj(&scope, moduleAt(&runtime, main, "obj"));
  EXPECT_EQ(*obj, Bool::falseObj());
}

TEST(BuiltinsModuleTest, HasAttrFromClassWithAttrReturnsTrue) {
  const char* src = R"(
class Foo:
  bar = 1
obj = hasattr(Foo, 'bar')
)";
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, src).isError());
  Module main(&scope, findModule(&runtime, "__main__"));
  Object obj(&scope, moduleAt(&runtime, main, "obj"));
  EXPECT_EQ(*obj, Bool::trueObj());
}

TEST(BuiltinsModuleTest, HasAttrWithNonStringAttrRaisesTypeError) {
  const char* src = R"(
class Foo:
  bar = 1
hasattr(Foo, 1)
)";
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, src), LayoutId::kTypeError,
                            "hasattr(): attribute name must be string"));
}

TEST(BuiltinsModuleTest, HasAttrFromInstanceMissingAttrReturnsFalse) {
  const char* src = R"(
class Foo: pass
obj = hasattr(Foo(), 'bar')
)";
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, src).isError());
  Module main(&scope, findModule(&runtime, "__main__"));
  Object obj(&scope, moduleAt(&runtime, main, "obj"));
  EXPECT_EQ(*obj, Bool::falseObj());
}

TEST(BuiltinsModuleTest, HasAttrFromInstanceWithAttrReturnsTrue) {
  const char* src = R"(
class Foo:
  bar = 1
obj = hasattr(Foo(), 'bar')
)";
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, src).isError());
  Module main(&scope, findModule(&runtime, "__main__"));
  Object obj(&scope, moduleAt(&runtime, main, "obj"));
  EXPECT_EQ(*obj, Bool::trueObj());
}

TEST(BuiltinsModuleTest,
     HashWithObjectWithNotCallableDunderHashRaisesTypeError) {
  const char* src = R"(
class C:
  __hash__ = None

hash(C())
)";
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, src), LayoutId::kTypeError,
                            "unhashable type: 'C'"));
}

TEST(BuiltinsModuleTest, HashWithObjectReturningNonIntRaisesTypeError) {
  const char* src = R"(
class C:
  def __hash__(self): return "10"

hash(C())
)";
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, src), LayoutId::kTypeError,
                            "__hash__ method should return an integer"));
}

TEST(BuiltinsModuleTest, HashWithObjectReturnsObjectDunderHashValue) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class C:
  def __hash__(self): return 10

h = hash(C())
)")
                   .isError());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "h"), SmallInt::fromWord(10));
}

TEST(BuiltinsModuleTest,
     HashWithObjectWithModifiedDunderHashReturnsClassDunderHashValue) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class C:
  def __hash__(self): return 10

def fake_hash(): return 0
c = C()
c.__hash__ = fake_hash
h = hash(c)
)")
                   .isError());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "h"), SmallInt::fromWord(10));
}

TEST(BuiltinsModuleTest, BuiltInSetAttr) {
  const char* src = R"(
class Foo:
  bar = 1
a = setattr(Foo, 'foo', 2)
b = Foo.foo
)";
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, src).isError());
  Module main(&scope, findModule(&runtime, "__main__"));
  Object a(&scope, moduleAt(&runtime, main, "a"));
  Object b(&scope, moduleAt(&runtime, main, "b"));
  EXPECT_EQ(*a, NoneType::object());
  EXPECT_EQ(*b, SmallInt::fromWord(2));
}

TEST(BuiltinsModuleTest, BuiltInSetAttrRaisesTypeError) {
  const char* src = R"(
class Foo:
  bar = 1
a = setattr(Foo, 2, 'foo')
)";
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, src), LayoutId::kTypeError,
                            "setattr(): attribute name must be string"));
}

TEST(BuiltinsModuleTest, ModuleAttrReturnsBuiltinsName) {
  // TODO(eelizondo): Parameterize test for all builtin types
  const char* src = R"(
a = hasattr(object, '__module__')
b = getattr(object, '__module__')
c = hasattr(list, '__module__')
d = getattr(list, '__module__')
)";
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, src).isError());

  Object a(&scope, moduleAt(&runtime, "__main__", "a"));
  EXPECT_EQ(*a, Bool::trueObj());
  Object b(&scope, moduleAt(&runtime, "__main__", "b"));
  ASSERT_TRUE(b.isStr());
  EXPECT_TRUE(Str::cast(*b).equalsCStr("builtins"));

  Object c(&scope, moduleAt(&runtime, "__main__", "c"));
  EXPECT_EQ(*c, Bool::trueObj());
  Object d(&scope, moduleAt(&runtime, "__main__", "d"));
  ASSERT_TRUE(d.isStr());
  EXPECT_TRUE(Str::cast(*b).equalsCStr("builtins"));
}

TEST(BuiltinsModuleTest, QualnameAttrReturnsTypeName) {
  // TODO(eelizondo): Parameterize test for all builtin types
  const char* src = R"(
a = hasattr(object, '__qualname__')
b = getattr(object, '__qualname__')
c = hasattr(list, '__qualname__')
d = getattr(list, '__qualname__')
)";
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, src).isError());

  Object a(&scope, moduleAt(&runtime, "__main__", "a"));
  EXPECT_EQ(*a, Bool::trueObj());
  Object b(&scope, moduleAt(&runtime, "__main__", "b"));
  ASSERT_TRUE(b.isStr());
  EXPECT_TRUE(Str::cast(*b).equalsCStr("object"));

  Object c(&scope, moduleAt(&runtime, "__main__", "c"));
  EXPECT_EQ(*c, Bool::trueObj());
  Object d(&scope, moduleAt(&runtime, "__main__", "d"));
  ASSERT_TRUE(d.isStr());
  EXPECT_TRUE(Str::cast(*d).equalsCStr("list"));
}

TEST(BuiltinsModuleTest, BuiltinCompile) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(
      runFromCStr(
          &runtime,
          R"(code = compile("a = 1\nb = 2", "<string>", "eval", dont_inherit=True))")
          .isError());
  Str filename(&scope, runtime.newStrFromCStr("<string>"));
  Code code(&scope, moduleAt(&runtime, "__main__", "code"));
  ASSERT_TRUE(code.filename().isStr());
  EXPECT_TRUE(Str::cast(code.filename()).equals(*filename));

  ASSERT_TRUE(code.names().isTuple());
  Tuple names(&scope, code.names());
  ASSERT_EQ(names.length(), 2);
  ASSERT_TRUE(names.contains(runtime.newStrFromCStr("a")));
  ASSERT_TRUE(names.contains(runtime.newStrFromCStr("b")));
}

TEST(BuiltinsModuleTest, BuiltinCompileBytes) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
data = b'a = 1; b = 2'
code = compile(data, "<string>", "eval", dont_inherit=True)
)")
                   .isError());
  Code code(&scope, moduleAt(&runtime, "__main__", "code"));
  Object filename(&scope, code.filename());
  EXPECT_TRUE(isStrEqualsCStr(*filename, "<string>"));

  ASSERT_TRUE(code.names().isTuple());
  Tuple names(&scope, code.names());
  ASSERT_EQ(names.length(), 2);
  ASSERT_TRUE(names.contains(runtime.newStrFromCStr("a")));
  ASSERT_TRUE(names.contains(runtime.newStrFromCStr("b")));
}

TEST(BuiltinsModuleTest, BuiltinCompileWithBytesSubclass) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class Foo(bytes): pass
data = Foo(b"a = 1; b = 2")
code = compile(data, "<string>", "eval", dont_inherit=True)
)")
                   .isError());
  Code code(&scope, moduleAt(&runtime, "__main__", "code"));
  Object filename(&scope, code.filename());
  EXPECT_TRUE(isStrEqualsCStr(*filename, "<string>"));

  ASSERT_TRUE(code.names().isTuple());
  Tuple names(&scope, code.names());
  ASSERT_EQ(names.length(), 2);
  ASSERT_TRUE(names.contains(runtime.newStrFromCStr("a")));
  ASSERT_TRUE(names.contains(runtime.newStrFromCStr("b")));
}

TEST(BuiltinsModuleDeathTest, BuiltinCompileRaisesTypeErrorGivenTooFewArgs) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, "compile(1)"), LayoutId::kTypeError,
      "TypeError: 'compile' takes min 3 positional arguments but 1 given"));
}

TEST(BuiltinsModuleDeathTest, BuiltinCompileRaisesTypeErrorGivenTooManyArgs) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, "compile(1, 2, 3, 4, 5, 6, 7, 8, 9)"),
      LayoutId::kTypeError,
      "TypeError: 'compile' takes max 6 positional arguments but 9 given"));
}

TEST(BuiltinsModuleTest, BuiltinCompileRaisesTypeErrorGivenBadMode) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime,
                  "compile('hello', 'hello', 'hello', dont_inherit=True)"),
      LayoutId::kValueError,
      "Expected mode to be 'exec', 'eval', or 'single' in 'compile'"));
}

TEST(BuiltinsModuleTest, BuiltinExecSetsGlobal) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
a = 1337
exec("a = 1338")
  )")
                   .isError());
  // We can't use runBuiltin here because it does not set up the frame properly
  // for functions that need globals, implicitGlobals, etc
  Object a(&scope, moduleAt(&runtime, "__main__", "a"));
  EXPECT_TRUE(isIntEqualsWord(*a, 1338));
}

TEST(BuiltinsModuleTest, BuiltinExecSetsGlobalGivenGlobals) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, "").isError());
  Module main(&scope, findModule(&runtime, "__main__"));
  Dict globals(&scope, main.dict());
  Str globals_name(&scope, runtime.newStrFromCStr("gl"));
  runtime.moduleDictAtPut(thread, globals, globals_name, globals);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
a = 1337
result = exec("a = 1338", gl)
  )")
                   .isError());
  Object result(&scope, moduleAt(&runtime, main, "result"));
  ASSERT_TRUE(result.isNoneType());
  Object a(&scope, moduleAt(&runtime, main, "a"));
  EXPECT_TRUE(isIntEqualsWord(*a, 1338));
}

TEST(BuiltinsModuleTest, BuiltinExecWithEmptyGlobalsFailsToSetGlobal) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
a = 1337
result = exec("a = 1338", {})
  )")
                   .isError());
  Module main(&scope, findModule(&runtime, "__main__"));
  Object result(&scope, moduleAt(&runtime, main, "result"));
  ASSERT_TRUE(result.isNoneType());
  Object a(&scope, moduleAt(&runtime, main, "a"));
  EXPECT_TRUE(isIntEqualsWord(*a, 1337));
}

TEST(BuiltinsModuleDeathTest, BuiltinExecWithNonDictGlobalsRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
a = 1337
result = exec("a = 1338", 7)
  )"),
                            LayoutId::kTypeError,
                            "Expected 'globals' to be dict in 'exec'"));
}

TEST(BuiltinsModuleTest, BuiltinExecExWithTupleCallsExec) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
exec(*("a = 1338",))
  )")
                   .isError());
  Object a(&scope, moduleAt(&runtime, "__main__", "a"));
  EXPECT_TRUE(isIntEqualsWord(*a, 1338));
}

TEST(BuiltinsModuleTest, BuiltinExecExWithListCallsExec) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
exec(*["a = 1338"])
  )")
                   .isError());
  Object a(&scope, moduleAt(&runtime, "__main__", "a"));
  EXPECT_TRUE(isIntEqualsWord(*a, 1338));
}

TEST(BuiltinsModuleTest, CopyFunctionEntriesCopies) {
  Runtime runtime;
  HandleScope scope;
  Function::Entry entry = BuiltinsModule::chr;
  Str qualname(&scope, runtime.symbols()->Chr());
  Function func(&scope,
                runtime.newBuiltinFunction(SymbolId::kChr, qualname, entry));
  ASSERT_FALSE(runFromCStr(&runtime, R"(
def chr(self):
  "docstring"
  pass
)")
                   .isError());
  Function python_func(&scope, moduleAt(&runtime, "__main__", "chr"));
  copyFunctionEntries(Thread::current(), func, python_func);
  Code base_code(&scope, func.code());
  Code patch_code(&scope, python_func.code());
  EXPECT_EQ(patch_code.code(), base_code.code());
  EXPECT_EQ(python_func.entry(), &builtinTrampoline);
  EXPECT_EQ(python_func.entryKw(), &builtinTrampolineKw);
  EXPECT_EQ(python_func.entryEx(), &builtinTrampolineEx);
}

TEST(BuiltinsModuleDeathTest, CopyFunctionEntriesRedefinitionDies) {
  Runtime runtime;
  HandleScope scope;
  Function::Entry entry = BuiltinsModule::chr;
  Str qualname(&scope, runtime.symbols()->Chr());
  Function func(&scope,
                runtime.newBuiltinFunction(SymbolId::kChr, qualname, entry));
  ASSERT_FALSE(runFromCStr(&runtime, R"(
def chr(self):
  return 42
)")
                   .isError());
  Function python_func(&scope, moduleAt(&runtime, "__main__", "chr"));
  ASSERT_DEATH(copyFunctionEntries(Thread::current(), func, python_func),
               "Redefinition of native code method 'chr' in managed code");
}

TEST(BuiltinsModuleTest,
     UnderIntNewFromByteArrayWithZeroBaseReturnsCodeLiteral) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  const byte view[] = {'0', 'x', 'b', 'a', '5', 'e'};
  Type type(&scope, runtime.typeAt(LayoutId::kInt));
  ByteArray array(&scope, runtime.newByteArray());
  runtime.byteArrayExtend(thread, array, view);
  Int base(&scope, SmallInt::fromWord(0));
  Object result(&scope, runBuiltin(BuiltinsModule::underIntNewFromByteArray,
                                   type, array, base));
  EXPECT_TRUE(isIntEqualsWord(*result, 0xba5e));
}

TEST(BuiltinsModuleTest,
     UnderIntNewFromByteArrayWithInvalidByteRaisesValueError) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  const byte view[] = {'$'};
  Type type(&scope, runtime.typeAt(LayoutId::kInt));
  ByteArray array(&scope, runtime.newByteArray());
  runtime.byteArrayExtend(thread, array, view);
  Int base(&scope, SmallInt::fromWord(36));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(BuiltinsModule::underIntNewFromByteArray, type, array, base),
      LayoutId::kValueError, "invalid literal for int() with base 36: b'$'"));
}

TEST(BuiltinsModuleTest,
     UnderIntNewFromByteArrayWithInvalidLiteraRaisesValueError) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  const byte view[] = {'a'};
  Type type(&scope, runtime.typeAt(LayoutId::kInt));
  ByteArray array(&scope, runtime.newByteArray());
  runtime.byteArrayExtend(thread, array, view);
  Int base(&scope, SmallInt::fromWord(10));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(BuiltinsModule::underIntNewFromByteArray, type, array, base),
      LayoutId::kValueError, "invalid literal for int() with base 10: b'a'"));
}

TEST(BuiltinsModuleTest, UnderIntNewFromBytesWithZeroBaseReturnsCodeLiteral) {
  Runtime runtime;
  HandleScope scope;
  const byte view[] = {'0', '4', '3'};
  Type type(&scope, runtime.typeAt(LayoutId::kInt));
  Bytes bytes(&scope, runtime.newBytesWithAll(view));
  Int base(&scope, SmallInt::fromWord(0));
  Object result(&scope, runBuiltin(BuiltinsModule::underIntNewFromBytes, type,
                                   bytes, base));
  EXPECT_TRUE(isIntEqualsWord(*result, 043));
}

TEST(BuiltinsModuleTest, UnderIntNewFromBytesWithInvalidByteRaisesValueError) {
  Runtime runtime;
  HandleScope scope;
  const byte view[] = {'$'};
  Type type(&scope, runtime.typeAt(LayoutId::kInt));
  Bytes bytes(&scope, runtime.newBytesWithAll(view));
  Int base(&scope, SmallInt::fromWord(36));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(BuiltinsModule::underIntNewFromBytes, type, bytes, base),
      LayoutId::kValueError, "invalid literal for int() with base 36: b'$'"));
}

TEST(BuiltinsModuleTest,
     UnderIntNewFromBytesWithInvalidLiteralRaisesValueError) {
  Runtime runtime;
  HandleScope scope;
  const byte view[] = {'8', '6'};
  Type type(&scope, runtime.typeAt(LayoutId::kInt));
  Bytes bytes(&scope, runtime.newBytesWithAll(view));
  Int base(&scope, SmallInt::fromWord(7));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(BuiltinsModule::underIntNewFromBytes, type, bytes, base),
      LayoutId::kValueError, "invalid literal for int() with base 7: b'86'"));
}

TEST(BuiltinsModuleTest, UnderIntNewFromBytesWithBytesSubclassReturnsSmallInt) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class Foo(bytes): pass
foo = Foo(b"42")
)")
                   .isError());
  Type type(&scope, runtime.typeAt(LayoutId::kInt));
  Bytes bytes(&scope, moduleAt(&runtime, "__main__", "foo"));
  Int base(&scope, SmallInt::fromWord(21));
  EXPECT_EQ(runBuiltin(BuiltinsModule::underIntNewFromBytes, type, bytes, base),
            SmallInt::fromWord(86));
}

TEST(BuiltinsModuleTest, UnderIntNewFromIntWithBoolReturnsSmallInt) {
  Runtime runtime;
  HandleScope scope;
  Object type(&scope, runtime.typeAt(LayoutId::kInt));
  Object fls(&scope, Bool::falseObj());
  Object tru(&scope, Bool::trueObj());
  Object false_result(
      &scope, runBuiltin(BuiltinsModule::underIntNewFromInt, type, fls));
  Object true_result(&scope,
                     runBuiltin(BuiltinsModule::underIntNewFromInt, type, tru));
  EXPECT_EQ(false_result, SmallInt::fromWord(0));
  EXPECT_EQ(true_result, SmallInt::fromWord(1));
}

TEST(BuiltinsModuleTest, UnderIntNewFromIntWithSubClassReturnsValueOfSubClass) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class SubInt(int):
  def __new__(cls, value):
      self = super(SubInt, cls).__new__(cls, value)
      self.name = "subint instance"
      return self

result = SubInt(50)
)")
                   .isError());
  HandleScope scope;
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_FALSE(result.isInt());
  EXPECT_TRUE(isIntEqualsWord(*result, 50));
}

TEST(BuiltinsModuleTest, UnderIntNewFromStrWithZeroBaseReturnsCodeLiteral) {
  Runtime runtime;
  HandleScope scope;
  const char* src = "1985";
  Type type(&scope, runtime.typeAt(LayoutId::kInt));
  Str str(&scope, runtime.newStrFromCStr(src));
  Int base(&scope, SmallInt::fromWord(0));
  Object result(
      &scope, runBuiltin(BuiltinsModule::underIntNewFromStr, type, str, base));
  EXPECT_TRUE(isIntEqualsWord(*result, 1985));
}

TEST(BuiltinsModuleTest, UnderIntNewFromStrWithInvalidCharRaisesValueError) {
  Runtime runtime;
  HandleScope scope;
  const char* src = "$";
  Type type(&scope, runtime.typeAt(LayoutId::kInt));
  Str str(&scope, runtime.newStrFromCStr(src));
  Int base(&scope, SmallInt::fromWord(36));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(BuiltinsModule::underIntNewFromStr, type, str, base),
      LayoutId::kValueError, "invalid literal for int() with base 36: '$'"));
}

TEST(BuiltinsModuleTest, UnderIntNewFromStrWithInvalidLiteralRaisesValueError) {
  Runtime runtime;
  HandleScope scope;
  const char* src = "305";
  Type type(&scope, runtime.typeAt(LayoutId::kInt));
  Str str(&scope, runtime.newStrFromCStr(src));
  Int base(&scope, SmallInt::fromWord(4));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(BuiltinsModule::underIntNewFromStr, type, str, base),
      LayoutId::kValueError, "invalid literal for int() with base 4: '305'"));
}

TEST(BuiltinsModuleDeathTest, UnderUnimplementedAbortsProgram) {
  Runtime runtime;
  ASSERT_DEATH(static_cast<void>(runFromCStr(&runtime, "_unimplemented()")),
               ".*'_unimplemented' called.");
}

TEST(BuiltinsModuleDeathTest, UnderUnimplementedPrintsFunctionName) {
  Runtime runtime;
  ASSERT_DEATH(static_cast<void>(runFromCStr(&runtime, R"(
def foobar():
  _unimplemented()
foobar()
)")),
               ".*'_unimplemented' called in function 'foobar'.");
}

TEST(BuiltinsModuleTest, UnderPatchWithBadPatchFuncRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  Object not_func(&scope, runtime.newInt(12));
  EXPECT_TRUE(raisedWithStr(runBuiltin(BuiltinsModule::underPatch, not_func),
                            LayoutId::kTypeError,
                            "_patch expects function argument"));
}

TEST(BuiltinsModuleTest, UnderPatchWithBadBaseFuncRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
not_a_function = 1234

@_patch
def not_a_function():
  pass
)"),
                            LayoutId::kTypeError,
                            "_patch can only patch functions"));
}

TEST(StrBuiltinsTest, UnderStrFromStrWithStrTypeReturnsValueOfStrType) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
result = _str_from_str(str, 'value')
)")
                   .isError());
  HandleScope scope;
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  ASSERT_TRUE(runtime.isInstanceOfStr(*result));
  EXPECT_TRUE(result.isStr());
}

TEST(StrBuiltinsTest,
     UnderStrFromStrWithSubClassTypeReturnsValueOfSubClassType) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class Sub(str): pass
result = _str_from_str(Sub, 'value')
)")
                   .isError());
  HandleScope scope;
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  Object sub(&scope, moduleAt(&runtime, "__main__", "Sub"));
  EXPECT_EQ(runtime.typeOf(*result), sub);
  EXPECT_TRUE(isStrEqualsCStr(*result, "value"));
}

TEST(StrArrayBuiltinsTest, UnderStrArrayIaddWithStrReturnsStrArray) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  StrArray self(&scope, runtime.newStrArray());
  const char* test_str = "hello";
  Str other(&scope, runtime.newStrFromCStr(test_str));
  StrArray result(&scope,
                  runBuiltin(BuiltinsModule::underStrArrayIadd, self, other));
  EXPECT_TRUE(isStrEqualsCStr(runtime.strFromStrArray(result), test_str));
  EXPECT_EQ(self, result);
}

TEST(BuiltinsModuleTest, AllOnListWithOnlyTrueReturnsTrue) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
result = all([True, True])
  )")
                   .isError());
  HandleScope scope;
  Bool result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_TRUE(result.value());
}

TEST(BuiltinsModuleTest, AllOnListWithFalseReturnsFalse) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
result = all([True, False, True])
  )")
                   .isError());
  HandleScope scope;
  Bool result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_FALSE(result.value());
}

TEST(BuiltinsModuleTest, AnyOnListWithOnlyFalseReturnsFalse) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
result = any([False, False])
  )")
                   .isError());
  HandleScope scope;
  Bool result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_FALSE(result.value());
}

TEST(BuiltinsModuleTest, AnyOnListWithTrueReturnsTrue) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
result = any([False, True, False])
  )")
                   .isError());
  HandleScope scope;
  Bool result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_TRUE(result.value());
}

TEST(BuiltinsModuleTest, RangeOnNonIntegerRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, R"(range("foo", "bar", "baz"))"),
      LayoutId::kTypeError, "Arguments to range() must be integers"));
}

TEST(BuiltinsModuleTest, RangeWithOnlyStopDefaultsOtherArguments) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
result = range(5)
  )")
                   .isError());
  HandleScope scope;
  Range result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_EQ(result.start(), 0);
  EXPECT_EQ(result.stop(), 5);
  EXPECT_EQ(result.step(), 1);
}

TEST(BuiltinsModuleTest, RangeWithStartAndStopDefaultsStep) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
result = range(1, 5)
  )")
                   .isError());
  HandleScope scope;
  Range result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_EQ(result.start(), 1);
  EXPECT_EQ(result.stop(), 5);
  EXPECT_EQ(result.step(), 1);
}

TEST(BuiltinsModuleTest, RangeWithAllArgsSetsAllArgs) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
result = range(1, 5, 7)
  )")
                   .isError());
  HandleScope scope;
  Range result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_EQ(result.start(), 1);
  EXPECT_EQ(result.stop(), 5);
  EXPECT_EQ(result.step(), 7);
}

TEST(BuiltinsModuleTest, FilterWithNonIterableArgumentRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, "filter(None, 1)"),
                            LayoutId::kTypeError,
                            "'int' object is not iterable"));
}

TEST(BuiltinsModuleTest,
     FilterWithNoneFuncAndIterableReturnsItemsOfTrueBoolValue) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
f = filter(None, [1,0,2,0])
r0 = f.__next__()
r1 = f.__next__()
exhausted = False
try:
  f.__next__()
except StopIteration:
  exhausted = True
)")
                   .isError());
  HandleScope scope;
  Object r0(&scope, moduleAt(&runtime, "__main__", "r0"));
  Object r1(&scope, moduleAt(&runtime, "__main__", "r1"));
  Object exhausted(&scope, moduleAt(&runtime, "__main__", "exhausted"));
  EXPECT_TRUE(isIntEqualsWord(*r0, 1));
  EXPECT_TRUE(isIntEqualsWord(*r1, 2));
  EXPECT_EQ(*exhausted, Bool::trueObj());
}

TEST(BuiltinsModuleTest,
     FilterWithFuncReturningBoolAndIterableReturnsItemsEvaluatedToTrueByFunc) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
def even(e): return e % 2 == 0

f = filter(even, [1,2,3,4])
r0 = f.__next__()
r1 = f.__next__()
exhausted = False
try:
  f.__next__()
except StopIteration:
  exhausted = True
)")
                   .isError());
  HandleScope scope;
  Object r0(&scope, moduleAt(&runtime, "__main__", "r0"));
  Object r1(&scope, moduleAt(&runtime, "__main__", "r1"));
  Object exhausted(&scope, moduleAt(&runtime, "__main__", "exhausted"));
  EXPECT_TRUE(isIntEqualsWord(*r0, 2));
  EXPECT_TRUE(isIntEqualsWord(*r1, 4));
  EXPECT_EQ(*exhausted, Bool::trueObj());
}

TEST(BuiltinsModuleTest, FormatWithNonStrFmtSpecRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(
      raised(runFromCStr(&runtime, "format('hi', 1)"), LayoutId::kTypeError));
}

TEST(BuiltinsModuleTest, FormatCallsDunderFormat) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class C:
  def __format__(self, fmt_spec):
    return "foobar"
result = format(C(), 'hi')
)")
                   .isError());
  EXPECT_TRUE(
      isStrEqualsCStr(moduleAt(&runtime, "__main__", "result"), "foobar"));
}

TEST(BuiltinsModuleTest, FormatRaisesWhenDunderFormatReturnsNonStr) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class C:
  def __format__(self, fmt_spec):
    return 1
)")
                   .isError());
  EXPECT_TRUE(
      raised(runFromCStr(&runtime, "format(C(), 'hi')"), LayoutId::kTypeError));
}

TEST(BuiltinsModuleTest, IterWithIterableCallsDunderIter) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
l = list(iter([1, 2, 3]))
)")
                   .isError());
  HandleScope scope;
  Object l(&scope, moduleAt(&runtime, "__main__", "l"));
  EXPECT_PYLIST_EQ(l, {1, 2, 3});
}

TEST(BuiltinsModuleTest, IterWithNonIterableRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
iter(None)
)"),
                            LayoutId::kTypeError,
                            "'NoneType' object is not iterable"));
}

TEST(BuiltinsModuleTest, IterWithRaisingDunderIterPropagatesException) {
  Runtime runtime;
  EXPECT_TRUE(raised(runFromCStr(&runtime, R"(
class C:
  def __iter__(self):
    raise UserWarning()
iter(C())
)"),
                     LayoutId::kUserWarning));
}

TEST(BuiltinsModuleTest, IterWithCallableReturnsIterator) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class C:
  def __init__(self):
    self.x = 0
  def __call__(self):
    self.x += 1
    return self.x
c = C()
callable_iter = iter(c, 3)
reduced = callable_iter.__reduce__()
l = list(callable_iter)
)")
                   .isError());
  HandleScope scope;
  Object l(&scope, moduleAt(&runtime, "__main__", "l"));
  EXPECT_PYLIST_EQ(l, {1, 2});

  Object iter(&scope, moduleAt(&runtime, "builtins", "iter"));
  Object c(&scope, moduleAt(&runtime, "__main__", "c"));
  Object reduced_obj(&scope, moduleAt(&runtime, "__main__", "reduced"));
  ASSERT_TRUE(reduced_obj.isTuple());
  Tuple reduced(&scope, *reduced_obj);
  ASSERT_EQ(reduced.length(), 2);
  EXPECT_EQ(reduced.at(0), iter);
  ASSERT_TRUE(reduced.at(1).isTuple());
  Tuple inner(&scope, reduced.at(1));
  ASSERT_EQ(inner.length(), 2);
  EXPECT_EQ(inner.at(0), c);
  EXPECT_TRUE(isIntEqualsWord(inner.at(1), 3));
}

TEST(BuiltinsModuleTest, NextWithoutIteratorRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
class C:
  pass
next(C())
)"),
                            LayoutId::kTypeError,
                            "'C' object is not iterable"));
}

TEST(BuiltinsModuleTest, NextWithIteratorFetchesNextItem) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class C:
  def __iter__(self):
    self.a = 1
    return self

  def __next__(self):
    x = self.a
    self.a += 1
    return x

itr = iter(C())
c = next(itr)
d = next(itr)
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(moduleAt(&runtime, "__main__", "c"), 1));
  EXPECT_TRUE(isIntEqualsWord(moduleAt(&runtime, "__main__", "d"), 2));
}

TEST(BuiltinsModuleTest, NextWithIteratorAndDefaultFetchesNextItem) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class C:
  def __iter__(self):
    self.a = 1
    return self

  def __next__(self):
    x = self.a
    self.a += 1
    return x

itr = iter(C())
c = next(itr, 0)
d = next(itr, 0)
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(moduleAt(&runtime, "__main__", "c"), 1));
  EXPECT_TRUE(isIntEqualsWord(moduleAt(&runtime, "__main__", "d"), 2));
}

TEST(BuiltinsModuleTest, NextWithIteratorRaisesStopIteration) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
class C:
  def __iter__(self):
    return self

  def __next__(self):
    raise StopIteration('stopit')

itr = iter(C())
next(itr)
)"),
                            LayoutId::kStopIteration, "stopit"));
}

TEST(BuiltinsModuleTest, NextWithIteratorAndDefaultReturnsDefault) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class C:
  def __iter__(self):
    return self

  def __next__(self):
    raise StopIteration('stopit')
itr = iter(C())
c = next(itr, None)
)")
                   .isError());
  EXPECT_TRUE(moduleAt(&runtime, "__main__", "c").isNoneType());
}

TEST(BuiltinsModuleTest, SortedReturnsSortedList) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
unsorted = [5, 7, 8, 6]
result = sorted(unsorted)
)")
                   .isError());

  HandleScope scope;
  Object unsorted_obj(&scope, moduleAt(&runtime, "__main__", "unsorted"));
  ASSERT_TRUE(unsorted_obj.isList());
  Object result_obj(&scope, moduleAt(&runtime, "__main__", "result"));
  ASSERT_TRUE(result_obj.isList());
  EXPECT_NE(*unsorted_obj, *result_obj);

  List unsorted(&scope, *unsorted_obj);
  ASSERT_EQ(unsorted.numItems(), 4);
  EXPECT_EQ(unsorted.at(0), SmallInt::fromWord(5));
  EXPECT_EQ(unsorted.at(1), SmallInt::fromWord(7));
  EXPECT_EQ(unsorted.at(2), SmallInt::fromWord(8));
  EXPECT_EQ(unsorted.at(3), SmallInt::fromWord(6));

  List result(&scope, *result_obj);
  ASSERT_EQ(result.numItems(), 4);
  EXPECT_EQ(result.at(0), SmallInt::fromWord(5));
  EXPECT_EQ(result.at(1), SmallInt::fromWord(6));
  EXPECT_EQ(result.at(2), SmallInt::fromWord(7));
  EXPECT_EQ(result.at(3), SmallInt::fromWord(8));
}

TEST(BuiltinsModuleTest, SortedWithReverseReturnsReverseSortedList) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
unsorted = [1, 2, 3, 4]
result = sorted(unsorted, reverse=True)
)")
                   .isError());

  HandleScope scope;
  Object unsorted_obj(&scope, moduleAt(&runtime, "__main__", "unsorted"));
  ASSERT_TRUE(unsorted_obj.isList());
  Object result_obj(&scope, moduleAt(&runtime, "__main__", "result"));
  ASSERT_TRUE(result_obj.isList());
  EXPECT_NE(*unsorted_obj, *result_obj);

  List unsorted(&scope, *unsorted_obj);
  ASSERT_EQ(unsorted.numItems(), 4);
  EXPECT_EQ(unsorted.at(0), SmallInt::fromWord(1));
  EXPECT_EQ(unsorted.at(1), SmallInt::fromWord(2));
  EXPECT_EQ(unsorted.at(2), SmallInt::fromWord(3));
  EXPECT_EQ(unsorted.at(3), SmallInt::fromWord(4));

  List result(&scope, *result_obj);
  ASSERT_EQ(result.numItems(), 4);
  EXPECT_EQ(result.at(0), SmallInt::fromWord(4));
  EXPECT_EQ(result.at(1), SmallInt::fromWord(3));
  EXPECT_EQ(result.at(2), SmallInt::fromWord(2));
  EXPECT_EQ(result.at(3), SmallInt::fromWord(1));
}

TEST(BuiltinsModuleTest, MaxWithEmptyIterableRaisesValueError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, "max([])"),
                            LayoutId::kValueError,
                            "max() arg is an empty sequence"));
}

TEST(BuiltinsModuleTest, MaxWithMultipleArgsReturnsMaximum) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, "result = max(1, 3, 5, 2, -1)").isError());
  EXPECT_TRUE(isIntEqualsWord(moduleAt(&runtime, "__main__", "result"), 5));
}

TEST(BuiltinsModuleTest, MaxWithNoArgsRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, "max()"), LayoutId::kTypeError,
      "TypeError: 'max' takes min 1 positional arguments but 0 given"));
}

TEST(BuiltinsModuleTest, MaxWithIterableReturnsMaximum) {
  Runtime runtime;
  ASSERT_FALSE(
      runFromCStr(&runtime, "result = max((1, 3, 5, 2, -1))").isError());
  EXPECT_TRUE(isIntEqualsWord(moduleAt(&runtime, "__main__", "result"), 5));
}

TEST(BuiltinsModuleTest, MaxWithEmptyIterableAndDefaultReturnsDefault) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, "result = max([], default=42)").isError());
  EXPECT_TRUE(isIntEqualsWord(moduleAt(&runtime, "__main__", "result"), 42));
}

TEST(BuiltinsModuleTest, MaxWithKeyOrdersByKeyFunction) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
result = max((1, 2, 3), key=lambda x: -x)
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(moduleAt(&runtime, "__main__", "result"), 1));
}

TEST(BuiltinsModuleTest, MaxWithEmptyIterableAndKeyAndDefaultReturnsDefault) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
result = max((), key=lambda x: x, default='empty')
)")
                   .isError());
  EXPECT_TRUE(
      isStrEqualsCStr(moduleAt(&runtime, "__main__", "result"), "empty"));
}

TEST(BuiltinsModuleTest, MaxWithMultipleArgsAndDefaultRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, "max(1, 2, default=0)"), LayoutId::kTypeError,
      "Cannot specify a default for max() with multiple positional arguments"));
}

TEST(BuiltinsModuleTest, MaxWithKeyReturnsFirstOccuranceOfEqualValues) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class A:
  pass

first = A()
second = A()
result = max(first, second, key=lambda x: 1) is first
)")
                   .isError());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "result"), Bool::trueObj());
}

TEST(BuiltinsModuleTest, MaxWithoutKeyReturnsFirstOccuranceOfEqualValues) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class A():
  def __gt__(self, _):
    return False

first = A()
second = A()
result = max(first, second) is first
)")
                   .isError());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "result"), Bool::trueObj());
}

TEST(BuiltinsModuleTest, MinWithEmptyIterableRaisesValueError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, "min([])"),
                            LayoutId::kValueError,
                            "min() arg is an empty sequence"));
}

TEST(BuiltinsModuleTest, MinWithMultipleArgsReturnsMinimum) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, "result = min(4, 3, 1, 2, 5)").isError());
  EXPECT_TRUE(isIntEqualsWord(moduleAt(&runtime, "__main__", "result"), 1));
}

TEST(BuiltinsModuleTest, MinWithNoArgsRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, "min()"), LayoutId::kTypeError,
      "TypeError: 'min' takes min 1 positional arguments but 0 given"));
}

TEST(BuiltinsModuleTest, MinWithIterableReturnsMinimum) {
  Runtime runtime;
  ASSERT_FALSE(
      runFromCStr(&runtime, "result = min((4, 3, 1, 2, 5))").isError());
  EXPECT_TRUE(isIntEqualsWord(moduleAt(&runtime, "__main__", "result"), 1));
}

TEST(BuiltinsModuleTest, MinWithEmptyIterableAndDefaultReturnsDefault) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, "result = min([], default=42)").isError());
  EXPECT_TRUE(isIntEqualsWord(moduleAt(&runtime, "__main__", "result"), 42));
}

TEST(BuiltinsModuleTest, MinWithKeyOrdersByKeyFunction) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
result = min((1, 2, 3), key=lambda x: -x)
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(moduleAt(&runtime, "__main__", "result"), 3));
}

TEST(BuiltinsModuleTest, MinWithEmptyIterableAndKeyAndDefaultReturnsDefault) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
result = min((), key=lambda x: x, default='empty')
)")
                   .isError());
  EXPECT_TRUE(
      isStrEqualsCStr(moduleAt(&runtime, "__main__", "result"), "empty"));
}

TEST(BuiltinsModuleTest, MinWithMultipleArgsAndDefaultRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, "min(1, 2, default=0)"), LayoutId::kTypeError,
      "Cannot specify a default for min() with multiple positional arguments"));
}

TEST(BuiltinsModuleTest, MinReturnsFirstOccuranceOfEqualValues) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class A:
  pass

first = A()
second = A()
result = min(first, second, key=lambda x: 1) is first
)")
                   .isError());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "result"), Bool::trueObj());
}

TEST(BuiltinsModuleTest, MinWithoutKeyReturnsFirstOccuranceOfEqualValues) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class A():
  def __lt__(self, _):
    return False

first = A()
second = A()
result = min(first, second) is first
)")
                   .isError());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "result"), Bool::trueObj());
}

TEST(BuiltinsModuleTest, MapWithNonIterableArgumentRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, "map(1,1)"),
                            LayoutId::kTypeError,
                            "'int' object is not iterable"));
}

TEST(BuiltinsModuleTest,
     MapWithIterableDunderNextReturnsFuncAppliedElementsSequentially) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
def inc(e):
  return e + 1

m = map(inc, [1,2])
r0 = m.__next__()
r1 = m.__next__()
)")
                   .isError());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "r0"), SmallInt::fromWord(2));
  EXPECT_EQ(moduleAt(&runtime, "__main__", "r1"), SmallInt::fromWord(3));
}

TEST(BuiltinsModuleTest,
     MapWithMultipleIterablesDunderNextReturnsFuncAppliedElementsSequentially) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
def inc(e0, e1):
  return e0 + e1

m = map(inc, [1,2], [100,200])
r0 = m.__next__()
r1 = m.__next__()
)")
                   .isError());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "r0"), SmallInt::fromWord(101));
  EXPECT_EQ(moduleAt(&runtime, "__main__", "r1"), SmallInt::fromWord(202));
}

TEST(BuiltinsModuleTest, MapDunderNextFinishesByRaisingStopIteration) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
def inc(e):
  return e + 1

m = map(inc, [1,2])
m.__next__()
m.__next__()
exc_raised = False
try:
  m.__next__()
except StopIteration:
  exc_raised = True
)")
                   .isError());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "exc_raised"), Bool::trueObj());
}

TEST(
    BuiltinsModuleTest,
    MapWithMultipleIterablesDunderNextFinishesByRaisingStopIterationOnShorterOne) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
def inc(e0, e1):
  return e0, e1

m = map(inc, [1,2], [100])
m.__next__()
exc_raised = False
try:
  m.__next__()
except StopIteration:
  exc_raised = True
)")
                   .isError());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "exc_raised"), Bool::trueObj());
}

TEST(BuiltinsModuleTest, EnumerateWithNonIterableRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, "enumerate(1.0)"),
                            LayoutId::kTypeError,
                            "'float' object is not iterable"));
}

TEST(BuiltinsModuleTest, EnumerateReturnsEnumeratedTuples) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
e = enumerate([7, 3])
res1 = e.__next__()
res2 = e.__next__()
exhausted = False
try:
  e.__next__()
except StopIteration:
  exhausted = True
)")
                   .isError());
  HandleScope scope;
  Object res1(&scope, moduleAt(&runtime, "__main__", "res1"));
  ASSERT_TRUE(res1.isTuple());
  EXPECT_EQ(Tuple::cast(*res1).at(0), SmallInt::fromWord(0));
  EXPECT_EQ(Tuple::cast(*res1).at(1), SmallInt::fromWord(7));
  Object res2(&scope, moduleAt(&runtime, "__main__", "res2"));
  ASSERT_TRUE(res2.isTuple());
  EXPECT_EQ(Tuple::cast(*res2).at(0), SmallInt::fromWord(1));
  EXPECT_EQ(Tuple::cast(*res2).at(1), SmallInt::fromWord(3));
  EXPECT_EQ(moduleAt(&runtime, "__main__", "exhausted"), Bool::trueObj());
}

TEST(BuiltinsModuleTest, AbsReturnsAbsoluteValue) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
res1 = abs(10)
res2 = abs(-10)
)")
                   .isError());

  HandleScope scope;
  Object res1(&scope, moduleAt(&runtime, "__main__", "res1"));
  EXPECT_TRUE(isIntEqualsWord(*res1, 10));
  Object res2(&scope, moduleAt(&runtime, "__main__", "res2"));
  EXPECT_TRUE(isIntEqualsWord(*res2, 10));
}

TEST(BuiltinsModuleTest, AbsWithoutDunderAbsRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
class Foo(): pass
res1 = abs(Foo())
)"),
                            LayoutId::kTypeError,
                            "bad operand type for abs(): 'Foo'"));
}

}  // namespace python

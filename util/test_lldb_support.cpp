#include <csignal>

#include "handles.h"
#include "runtime.h"

using namespace python;

int main(int argc, char* argv[]) {
  Runtime runtime;
  HandleScope scope;

  // This file is used by test_lldb_support.py, both to create values to
  // inspect in lldb and to provide the expected output of the printers in
  // lldb_support.py.
  //
  // Comment lines beginning with either "// exp: " or "// re: " provide exact
  // match or regex test patterns, respectively. The rest of the line is used
  // as the pattern, and if it matches a full line anywhere in the output from
  // lldb, that pattern passes.
  //
  // Note that this means that you could have the right output in the wrong
  // place and still pass all tests. It's not perfect but it's simple and it
  // gets the job done.

  // clang-format off

  // exp: (python::RawObject) imm1 = None
  RawObject imm1 = NoneType::object();
  // exp: (python::Object) imm2 = Error
  Object imm2(&scope, Error::object());
  // exp: (python::Object) imm3 = False
  Object imm3(&scope, Bool::falseObj());
  // exp: (python::Object) imm4 = True
  Object imm4(&scope, Bool::trueObj());
  // exp: (python::Object) imm5 = NotImplemented
  Object imm5(&scope, NotImplementedType::object());
  // exp: (python::Object) imm6 = Unbound
  Object imm6(&scope, Unbound::object());

  // exp: (python::RawSmallInt) int1 = 1234
  RawSmallInt int1 = SmallInt::fromWord(1234);
  // re: \(python::Int\) int2 = HeapObject @ 0x[0-9a-f]+ Header<kDataArray64, kLargeInt, hash=0, count=1>
  Int int2(&scope, runtime.newInt(SmallInt::kMaxValue + 1));

  // exp: (python::RawObject) str1 = SmallStr('short')
  RawObject str1 = SmallStr::fromCStr("short");
  // re: \(python::Str\) str2 = HeapObject @ 0x[0-9a-f]+ Header<kDataArray8, kLargeStr, hash=0, count=15>
  Str str2(&scope, runtime.newStrFromCStr("a longer string"));

  // re: \(python::RawObject\) heap1 = HeapObject @ 0x[0-9a-f]+ Header<kObjectArray, kTuple, hash=0, count=10>
  RawObject heap1 = runtime.newTuple(10);
  // re: \(python::HeapObject\) heap2 = HeapObject @ 0x[0-9a-f]+ Header<kObjectInstance, kList, hash=0, count=2>
  HeapObject heap2(&scope, runtime.newList());

  // clang-format on
  std::raise(SIGINT);
  return 0;
}

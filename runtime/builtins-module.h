#pragma once

#include "frame.h"
#include "globals.h"
#include "modules.h"
#include "objects.h"
#include "runtime.h"

namespace py {

RawObject delAttribute(Thread* thread, const Object& self, const Object& name);
RawObject getAttribute(Thread* thread, const Object& self, const Object& name);
RawObject hasAttribute(Thread* thread, const Object& self, const Object& name);
RawObject setAttribute(Thread* thread, const Object& self, const Object& name,
                       const Object& value);

RawObject compile(Thread* thread, const Object& source, const Object& filename,
                  SymbolId mode, word flags, int optimize);

class BuiltinsModule {
 public:
  static void initialize(Thread* thread, const Module& module);

  static RawObject bin(Thread* thread, Frame* frame, word nargs);
  static RawObject callable(Thread* thread, Frame* frame, word nargs);
  static RawObject chr(Thread* thread, Frame* frame, word nargs);
  static RawObject delattr(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderBuildClass(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderImport(Thread* thread, Frame* frame, word nargs);
  static RawObject getattr(Thread* thread, Frame* frame, word nargs);
  static RawObject hasattr(Thread* thread, Frame* frame, word nargs);
  static RawObject hash(Thread* thread, Frame* frame, word nargs);
  static RawObject hex(Thread* thread, Frame* frame, word nargs);
  static RawObject id(Thread* thread, Frame* frame, word nargs);
  static RawObject oct(Thread* thread, Frame* frame, word nargs);
  static RawObject ord(Thread* thread, Frame* frame, word nargs);
  static RawObject setattr(Thread* thread, Frame* frame, word nargs);

 private:
  static const BuiltinFunction kBuiltinFunctions[];
  static const BuiltinType kBuiltinTypes[];
  static const SymbolId kIntrinsicIds[];
};

}  // namespace py

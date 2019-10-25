#pragma once

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace py {

class ClassMethodBuiltins
    : public Builtins<ClassMethodBuiltins, SymbolId::kClassmethod,
                      LayoutId::kClassMethod> {
 public:
  static RawObject dunderNew(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderInit(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderGet(Thread* thread, Frame* frame, word nargs);

  static const BuiltinMethod kBuiltinMethods[];
};

class StaticMethodBuiltins
    : public Builtins<StaticMethodBuiltins, SymbolId::kStaticMethod,
                      LayoutId::kStaticMethod> {
 public:
  static RawObject dunderNew(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderInit(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderGet(Thread* thread, Frame* frame, word nargs);

  static const BuiltinMethod kBuiltinMethods[];
};

class PropertyBuiltins : public Builtins<PropertyBuiltins, SymbolId::kProperty,
                                         LayoutId::kProperty> {
 public:
  static RawObject dunderDelete(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderGet(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderInit(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderNew(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderSet(Thread* thread, Frame* frame, word nargs);
  static RawObject deleter(Thread* thread, Frame* frame, word nargs);
  static RawObject getter(Thread* thread, Frame* frame, word nargs);
  static RawObject setter(Thread* thread, Frame* frame, word nargs);

  static const BuiltinMethod kBuiltinMethods[];
};

}  // namespace py

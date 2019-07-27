#pragma once

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace python {

enum class ExtensionMethodType {
  kMethNoArgs = 4,
  kMethO = 8,
  kMethVarArgs = 1,
  kMethVarArgsAndKeywords = 3,
};

RawObject functionFromMethodDef(Thread* thread, const char* c_name, void* meth,
                                const char* c_doc, ExtensionMethodType type);

RawObject functionFromModuleMethodDef(Thread* thread, const char* c_name,
                                      void* meth, const char* c_doc,
                                      ExtensionMethodType type);

RawObject functionGetAttribute(Thread* thread, const Function& function,
                               const Object& name_str);

RawObject functionSetAttr(Thread* thread, const Function& function,
                          const Object& name_interned_str, const Object& value);

class FunctionBuiltins : public Builtins<FunctionBuiltins, SymbolId::kFunction,
                                         LayoutId::kFunction> {
 public:
  static void postInitialize(Runtime* runtime, const Type& new_type);

  static RawObject dunderGet(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderGetattribute(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderSetattr(Thread* thread, Frame* frame, word nargs);

  static const BuiltinMethod kBuiltinMethods[];
  static const BuiltinAttribute kAttributes[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(FunctionBuiltins);
};

class BoundMethodBuiltins
    : public Builtins<BoundMethodBuiltins, SymbolId::kMethod,
                      LayoutId::kBoundMethod> {
 public:
  static const BuiltinAttribute kAttributes[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(BoundMethodBuiltins);
};

}  // namespace python

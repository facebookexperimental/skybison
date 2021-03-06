// Copyright (c) Facebook, Inc. and its affiliates. (http://www.facebook.com)
#include "function-builtins.h"

#include "builtins.h"
#include "dict-builtins.h"
#include "frame.h"
#include "globals.h"
#include "object-builtins.h"
#include "objects.h"
#include "runtime.h"
#include "str-builtins.h"
#include "thread.h"
#include "type-builtins.h"

namespace py {

static const BuiltinAttribute kFunctionAttributes[] = {
    // TODO(T44845145) Support assignment to __code__.
    {ID(__code__), RawFunction::kCodeOffset, AttributeFlags::kReadOnly},
    {ID(_function__flags), RawFunction::kFlagsOffset, AttributeFlags::kHidden},
    {ID(_function__argcount), RawFunction::kArgcountOffset,
     AttributeFlags::kHidden},
    {ID(_function__total_args), RawFunction::kTotalArgsOffset,
     AttributeFlags::kHidden},
    {ID(_function__total_vars), RawFunction::kTotalVarsOffset,
     AttributeFlags::kHidden},
    {ID(_function__stack_size), RawFunction::kStacksizeOrBuiltinOffset,
     AttributeFlags::kHidden},
    {ID(__doc__), RawFunction::kDocOffset},
    {ID(__name__), RawFunction::kNameOffset},
    {ID(__qualname__), RawFunction::kQualnameOffset},
    {ID(__module__), RawFunction::kModuleNameOffset},
    {ID(__module_object__), RawFunction::kModuleObjectOffset},
    {ID(_function__defaults), RawFunction::kDefaultsOffset,
     AttributeFlags::kHidden},
    {ID(_function__annotations), RawFunction::kAnnotationsOffset,
     AttributeFlags::kHidden},
    {ID(_function__kw_defaults), RawFunction::kKwDefaultsOffset,
     AttributeFlags::kHidden},
    {ID(_function__closure), RawFunction::kClosureOffset,
     AttributeFlags::kHidden},
    {ID(_function__entry), RawFunction::kEntryOffset, AttributeFlags::kHidden},
    {ID(_function__entry_kw), RawFunction::kEntryKwOffset,
     AttributeFlags::kHidden},
    {ID(_function__entry_ex), RawFunction::kEntryExOffset,
     AttributeFlags::kHidden},
    {ID(_function__entry_asm), RawFunction::kEntryAsmOffset,
     AttributeFlags::kHidden},
    {ID(_function__rewritten_bytecode), RawFunction::kRewrittenBytecodeOffset,
     AttributeFlags::kHidden},
    {ID(_function__caches), RawFunction::kCachesOffset,
     AttributeFlags::kHidden},
    {ID(_function__dict), RawFunction::kDictOffset, AttributeFlags::kHidden},
    {ID(_function__intrinsic), RawFunction::kIntrinsicOffset,
     AttributeFlags::kHidden},
};

static const BuiltinAttribute kBoundMethodAttributes[] = {
    {ID(__func__), RawBoundMethod::kFunctionOffset, AttributeFlags::kReadOnly},
    {ID(__self__), RawBoundMethod::kSelfOffset, AttributeFlags::kReadOnly},
};

static const BuiltinAttribute kInstanceMethodAttributes[] = {
    {ID(__func__), RawInstanceMethod::kFunctionOffset,
     AttributeFlags::kReadOnly},
};

void initializeFunctionTypes(Thread* thread) {
  HandleScope scope(thread);
  Type type(&scope, addBuiltinType(thread, ID(function), LayoutId::kFunction,
                                   /*superclass_id=*/LayoutId::kObject,
                                   kFunctionAttributes, Function::kSize,
                                   /*basetype=*/false));
  Layout layout(&scope, type.instanceLayout());
  layout.setDictOverflowOffset(RawFunction::kDictOffset);

  addBuiltinType(thread, ID(method), LayoutId::kBoundMethod,
                 /*superclass_id=*/LayoutId::kObject, kBoundMethodAttributes,
                 BoundMethod::kSize, /*basetype=*/false);

  addBuiltinType(thread, ID(instancemethod), LayoutId::kInstanceMethod,
                 /*superclass_id=*/LayoutId::kObject, kInstanceMethodAttributes,
                 InstanceMethod::kSize, /*basetype=*/false);
}

RawObject slotWrapperFunctionType(const Function& function) {
  DCHECK(
      !function.isInterpreted(),
      "slotWrapperFunctionType does not make sense for interpreted functions");
  // We misuse the rewrittenBytecode slot for extension functions (they do not
  // have bytecode).
  return function.rewrittenBytecode();
}

void slotWrapperFunctionSetType(const Function& function, const Type& type) {
  DCHECK(!function.isInterpreted(),
         "slotWrapperFunctionSetType does not make sense for interpreted "
         "functions");
  // We misuse the rewrittenBytecode slot for extension functions (they do not
  // have bytecode).
  function.setRewrittenBytecode(*type);
}

RawObject METH(function, __get__)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self.isFunction()) {
    return thread->raiseRequiresType(self, ID(function));
  }
  Object instance(&scope, args.get(1));
  // When `instance is None` return the plain function because we are doing a
  // lookup on a class.
  if (instance.isNoneType()) {
    if (args.get(2).isNoneType()) {
      return thread->raiseWithFmt(LayoutId::kTypeError,
                                  "__get__(None, None) is invalid");
    }
    return *self;
  }
  return thread->runtime()->newBoundMethod(self, instance);
}

}  // namespace py

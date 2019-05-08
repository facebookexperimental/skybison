#include "type-builtins.h"

#include "bytecode.h"
#include "frame.h"
#include "globals.h"
#include "mro.h"
#include "object-builtins.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"
#include "trampolines-inl.h"

namespace python {

RawObject typeLookupNameInMro(Thread* thread, const Type& type,
                              const Object& name_str) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Tuple mro(&scope, type.mro());
  for (word i = 0; i < mro.length(); i++) {
    Type mro_type(&scope, mro.at(i));
    Dict dict(&scope, mro_type.dict());
    Object value(&scope, runtime->typeDictAt(dict, name_str));
    if (!value.isError()) {
      return *value;
    }
  }
  return Error::notFound();
}

RawObject typeLookupSymbolInMro(Thread* thread, const Type& type,
                                SymbolId symbol) {
  HandleScope scope(thread);
  Object symbol_str(&scope, thread->runtime()->symbols()->at(symbol));
  return typeLookupNameInMro(thread, type, symbol_str);
}

bool typeIsDataDescriptor(Thread* thread, const Type& type) {
  // TODO(T25692962): Track "descriptorness" through a bit on the class
  HandleScope scope(thread);
  Object dunder_set_name(&scope, thread->runtime()->symbols()->DunderSet());
  return !typeLookupNameInMro(thread, type, dunder_set_name).isError();
}

bool typeIsNonDataDescriptor(Thread* thread, const Type& type) {
  // TODO(T25692962): Track "descriptorness" through a bit on the class
  HandleScope scope(thread);
  Object dunder_get_name(&scope, thread->runtime()->symbols()->DunderGet());
  return !typeLookupNameInMro(thread, type, dunder_get_name).isError();
}

RawObject typeGetAttribute(Thread* thread, const Type& type,
                           const Object& name_str) {
  // Look for the attribute in the meta class
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Type meta_type(&scope, runtime->typeOf(*type));
  Object meta_attr(&scope, typeLookupNameInMro(thread, meta_type, name_str));
  if (!meta_attr.isError()) {
    Type meta_attr_type(&scope, runtime->typeOf(*meta_attr));
    if (typeIsDataDescriptor(thread, meta_attr_type)) {
      return Interpreter::callDescriptorGet(thread, thread->currentFrame(),
                                            meta_attr, type, meta_type);
    }
  }

  // No data descriptor found on the meta class, look in the mro of the type
  Object attr(&scope, typeLookupNameInMro(thread, type, name_str));
  if (!attr.isError()) {
    Type attr_type(&scope, runtime->typeOf(*attr));
    if (typeIsNonDataDescriptor(thread, attr_type)) {
      // Unfortunately calling `__get__` for a lookup on `type(None)` will look
      // exactly the same as calling it for a lookup on the `None` object.
      // To solve the ambiguity we add a special case for `type(None)` here.
      // Luckily it is impossible for the user to change the type so we can
      // special case the desired lookup behavior here.
      // Also see `FunctionBuiltins::dunderGet` for the related special casing
      // of lookups on the `None` object.
      if (type.builtinBase() == LayoutId::kNoneType) {
        return *attr;
      }
      Object none(&scope, NoneType::object());
      return Interpreter::callDescriptorGet(thread, thread->currentFrame(),
                                            attr, none, type);
    }
    return *attr;
  }

  // No data descriptor found on the meta class, look on the type
  Object result(&scope, instanceGetAttribute(thread, type, name_str));
  if (!result.isError()) {
    return *result;
  }

  // No attr found in type or its mro, use the non-data descriptor found in
  // the metaclass (if any).
  if (!meta_attr.isError()) {
    Type meta_attr_type(&scope, runtime->typeOf(*meta_attr));
    if (typeIsNonDataDescriptor(thread, meta_attr_type)) {
      return Interpreter::callDescriptorGet(thread, thread->currentFrame(),
                                            meta_attr, type, meta_type);
    }
    // If a regular attribute was found in the metaclass, return it
    return *meta_attr;
  }

  return Error::notFound();
}

RawObject typeNew(Thread* thread, LayoutId metaclass_id, const Str& name,
                  const Tuple& bases, const Dict& dict) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Type type(&scope, runtime->newTypeWithMetaclass(metaclass_id));
  type.setName(*name);

  // Compute MRO
  Object maybe_mro(&scope, computeMro(thread, type, bases));
  if (maybe_mro.isError()) {
    return *maybe_mro;
  }
  type.setMro(*maybe_mro);

  // Initialize dict
  Object class_cell_key(&scope, runtime->symbols()->DunderClassCell());
  Object class_cell(&scope, runtime->dictAt(dict, class_cell_key));
  if (!class_cell.isError()) {
    RawValueCell::cast(RawValueCell::cast(*class_cell).value()).setValue(*type);
    runtime->dictRemove(dict, class_cell_key);
  }
  type.setDict(*dict);

  // Compute builtin base class
  Object builtin_base(&scope, runtime->computeBuiltinBase(thread, type));
  if (builtin_base.isError()) {
    return *builtin_base;
  }
  Type builtin_base_type(&scope, *builtin_base);
  LayoutId base_layout_id =
      RawLayout::cast(builtin_base_type.instanceLayout()).id();

  // Initialize instance layout
  Layout layout(&scope,
                runtime->computeInitialLayout(thread, type, base_layout_id));
  layout.setDescribedType(*type);
  type.setInstanceLayout(*layout);

  // Copy down class flags from bases
  Tuple mro(&scope, *maybe_mro);
  word flags = 0;
  for (word i = 1; i < mro.length(); i++) {
    Type cur(&scope, mro.at(i));
    flags |= cur.flags();
  }
  type.setFlagsAndBuiltinBase(static_cast<RawType::Flag>(flags),
                              base_layout_id);
  type.setBases(*bases);
  return *type;
}

RawObject typeSetAttr(Thread* thread, const Type& type,
                      const Object& name_interned_str, const Object& value) {
  Runtime* runtime = thread->runtime();
  DCHECK(runtime->isInternedStr(name_interned_str),
         "name must be an interned string");
  HandleScope scope(thread);
  if (type.isBuiltin()) {
    Object type_name(&scope, type.name());
    return thread->raiseWithFmt(
        LayoutId::kTypeError,
        "can't set attributes of built-in/extension type '%S'", &type_name);
  }

  // Check for a data descriptor
  Type metatype(&scope, runtime->typeOf(*type));
  Object meta_attr(&scope,
                   typeLookupNameInMro(thread, metatype, name_interned_str));
  if (!meta_attr.isError()) {
    Type meta_attr_type(&scope, runtime->typeOf(*meta_attr));
    if (typeIsDataDescriptor(thread, meta_attr_type)) {
      Object set_result(
          &scope, Interpreter::callDescriptorSet(thread, thread->currentFrame(),
                                                 meta_attr, type, value));
      if (set_result.isError()) return *set_result;
      return NoneType::object();
    }
  }

  // No data descriptor found, store the attribute in the type dict
  Dict type_dict(&scope, type.dict());
  runtime->typeDictAtPut(type_dict, name_interned_str, value);
  return NoneType::object();
}

const BuiltinAttribute TypeBuiltins::kAttributes[] = {
    {SymbolId::kDunderBases, RawType::kBasesOffset, AttributeFlags::kReadOnly},
    {SymbolId::kDunderDict, RawType::kDictOffset, AttributeFlags::kReadOnly},
    {SymbolId::kDunderDoc, RawType::kDocOffset},
    {SymbolId::kDunderFlags, RawType::kFlagsOffset, AttributeFlags::kReadOnly},
    {SymbolId::kDunderMro, RawType::kMroOffset, AttributeFlags::kReadOnly},
    {SymbolId::kDunderName, RawType::kNameOffset},
    {SymbolId::kSentinelId, -1},
};

const BuiltinMethod TypeBuiltins::kBuiltinMethods[] = {
    {SymbolId::kDunderCall, dunderCall},
    {SymbolId::kDunderGetattribute, dunderGetattribute},
    {SymbolId::kDunderNew, dunderNew},
    {SymbolId::kDunderSetattr, dunderSetattr},
    {SymbolId::kSentinelId, nullptr},
};

void TypeBuiltins::postInitialize(Runtime* /* runtime */,
                                  const Type& new_type) {
  HandleScope scope;
  Layout layout(&scope, new_type.instanceLayout());
  layout.setOverflowAttributes(SmallInt::fromWord(RawType::kDictOffset));
}

RawObject userVisibleTypeOf(Thread* thread, const Object& obj) {
  LayoutId id = obj.layoutId();
  // Hide our size-specific type implementations from the user.
  switch (id) {
    case LayoutId::kSmallStr:
    case LayoutId::kLargeStr:
      id = LayoutId::kStr;
      break;
    case LayoutId::kSmallInt:
    case LayoutId::kLargeInt:
      id = LayoutId::kInt;
      break;
    default:
      break;
  }
  return thread->runtime()->typeAt(id);
}

RawObject TypeBuiltins::dunderCall(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object metaclass_obj(&scope, args.get(0));
  Tuple pargs(&scope, args.get(1));
  Dict kwargs(&scope, args.get(2));
  // Shortcut for type(x) calls.
  if (pargs.length() == 1 && kwargs.numItems() == 0 &&
      metaclass_obj == runtime->typeAt(LayoutId::kType)) {
    Object obj(&scope, pargs.at(0));
    return userVisibleTypeOf(thread, obj);
  }

  if (!runtime->isInstanceOfType(*metaclass_obj)) {
    return thread->raiseTypeErrorWithCStr("self must be a type instance");
  }
  Type metaclass(&scope, *metaclass_obj);

  Object dunder_new(
      &scope, runtime->attributeAtId(thread, metaclass, SymbolId::kDunderNew));
  CHECK(!dunder_new.isError(), "metaclass must have __new__");
  frame->pushValue(*dunder_new);
  Tuple call_args(&scope, runtime->newTuple(pargs.length() + 1));
  call_args.atPut(0, *metaclass);
  for (word i = 0, length = pargs.length(); i < length; i++) {
    call_args.atPut(i + 1, pargs.at(i));
  }
  frame->pushValue(*call_args);
  frame->pushValue(*kwargs);
  Object instance(&scope, Interpreter::callEx(
                              thread, frame, CallFunctionExFlag::VAR_KEYWORDS));
  if (instance.isError()) return *instance;
  if (!runtime->isInstance(instance, metaclass)) {
    return *instance;
  }

  Object dunder_init(
      &scope, runtime->attributeAtId(thread, metaclass, SymbolId::kDunderInit));
  CHECK(!dunder_init.isError(), "metaclass must have __init__");
  frame->pushValue(*dunder_init);
  call_args.atPut(0, *instance);
  frame->pushValue(*call_args);
  frame->pushValue(*kwargs);
  Object result(&scope, Interpreter::callEx(thread, frame,
                                            CallFunctionExFlag::VAR_KEYWORDS));
  if (result.isError()) return *result;
  if (!result.isNoneType()) {
    Object type_name(&scope, metaclass.name());
    return thread->raiseTypeError(
        runtime->newStrFromFmt("%S.__init__ returned non None", &type_name));
  }
  return *instance;
}

RawObject TypeBuiltins::dunderGetattribute(Thread* thread, Frame* frame,
                                           word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfType(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kType);
  }
  Type self(&scope, *self_obj);
  Object name(&scope, args.get(1));
  if (!runtime->isInstanceOfStr(*name)) {
    return thread->raiseWithFmt(
        LayoutId::kTypeError, "attribute name must be string, not '%T'", &name);
  }
  Object result(&scope, typeGetAttribute(thread, self, name));
  if (result.isErrorNotFound()) {
    Object type_name(&scope, self.name());
    return thread->raiseWithFmt(LayoutId::kAttributeError,
                                "type object '%S' has no attribute '%S'",
                                &type_name, &name);
  }
  return *result;
}

RawObject TypeBuiltins::dunderNew(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Type metaclass(&scope, args.get(0));
  LayoutId metaclass_id = RawLayout::cast(metaclass.instanceLayout()).id();
  // If the first argument is exactly type, and there are no other arguments,
  // then this call acts like a "typeof" operator, and returns the type of the
  // argument.
  if (args.get(2).isUnbound() && args.get(3).isUnbound() &&
      metaclass_id == LayoutId::kType) {
    Object arg(&scope, args.get(1));
    return userVisibleTypeOf(thread, arg);
  }
  Str name(&scope, args.get(1));
  Tuple bases(&scope, args.get(2));
  Dict dict(&scope, args.get(3));
  return typeNew(thread, metaclass_id, name, bases, dict);
}

RawObject TypeBuiltins::dunderSetattr(Thread* thread, Frame* frame,
                                      word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfType(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kType);
  }
  Type self(&scope, *self_obj);
  Object name(&scope, args.get(1));
  if (!runtime->isInstanceOfStr(*name)) {
    return thread->raiseWithFmt(
        LayoutId::kTypeError, "attribute name must be string, not '%T'", &name);
  }
  if (!name.isStr()) {
    UNIMPLEMENTED("Strict subclass of string");
  }
  name = runtime->internStr(name);
  Object value(&scope, args.get(2));
  return typeSetAttr(thread, self, name, value);
}

}  // namespace python

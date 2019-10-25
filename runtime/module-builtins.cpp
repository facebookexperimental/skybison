#include "module-builtins.h"

#include "capi-handles.h"
#include "frame.h"
#include "globals.h"
#include "ic.h"
#include "object-builtins.h"
#include "objects.h"
#include "runtime.h"
#include "str-builtins.h"
#include "thread.h"

namespace py {

static RawObject unwrapValueCell(RawObject result) {
  if (result.isErrorNotFound()) {
    return result;
  }
  RawValueCell value_cell = ValueCell::cast(result);
  if (value_cell.isPlaceholder()) {
    return Error::notFound();
  }
  return value_cell.value();
}

RawObject moduleAt(Thread* thread, const Module& module, const Object& key,
                   const Object& key_hash) {
  HandleScope scope(thread);
  Dict dict(&scope, module.dict());
  return unwrapValueCell(
      thread->runtime()->dictAt(thread, dict, key, key_hash));
}

RawObject moduleAtByStr(Thread* thread, const Module& module, const Str& name) {
  HandleScope scope(thread);
  Dict dict(&scope, module.dict());
  return unwrapValueCell(thread->runtime()->dictAtByStr(thread, dict, name));
}

RawObject moduleAtById(Thread* thread, const Module& module, SymbolId id) {
  HandleScope scope(thread);
  Dict dict(&scope, module.dict());
  return unwrapValueCell(thread->runtime()->dictAtById(thread, dict, id));
}

static RawObject filterPlaceholderValueCell(RawObject result) {
  if (result.isErrorNotFound()) {
    return result;
  }
  RawValueCell value_cell = ValueCell::cast(result);
  if (value_cell.isPlaceholder()) {
    return Error::notFound();
  }
  return value_cell;
}

RawObject moduleValueCellAtById(Thread* thread, const Module& module,
                                SymbolId id) {
  HandleScope scope(thread);
  Dict dict(&scope, module.dict());
  return filterPlaceholderValueCell(
      thread->runtime()->dictAtById(thread, dict, id));
}

RawObject moduleValueCellAtByStr(Thread* thread, const Module& module,
                                 const Str& name) {
  HandleScope scope(thread);
  Dict dict(&scope, module.dict());
  return filterPlaceholderValueCell(
      thread->runtime()->dictAtByStr(thread, dict, name));
}

static RawObject moduleValueCellAtPut(Thread* thread, const Module& module,
                                      const Object& key, const Object& key_hash,
                                      const Object& value) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Dict module_dict(&scope, module.dict());
  Object module_result(&scope,
                       runtime->dictAt(thread, module_dict, key, key_hash));
  if (module_result.isValueCell() &&
      ValueCell::cast(*module_result).isPlaceholder()) {
    // A builtin entry is cached under the same key, so invalidate its caches.
    Module builtins_module(
        &scope, moduleAtById(thread, module, SymbolId::kDunderBuiltins));
    Dict builtins_dict(&scope, builtins_module.dict());
    Object builtins_result(&scope,
                           filterPlaceholderValueCell(thread->runtime()->dictAt(
                               thread, builtins_dict, key, key_hash)));
    DCHECK(builtins_result.isValueCell(), "a builtin entry must exist");
    ValueCell builtins_value_cell(&scope, *builtins_result);
    DCHECK(!builtins_value_cell.dependencyLink().isNoneType(),
           "the builtin valuecell must have a dependent");
    icInvalidateGlobalVar(thread, builtins_value_cell);
  }
  return thread->runtime()->dictAtPutInValueCell(thread, module_dict, key,
                                                 key_hash, value);
}

RawObject moduleAtPut(Thread* thread, const Module& module, const Object& key,
                      const Object& key_hash, const Object& value) {
  return moduleValueCellAtPut(thread, module, key, key_hash, value);
}

RawObject moduleAtPutByStr(Thread* thread, const Module& module,
                           const Str& name, const Object& value) {
  HandleScope scope(thread);
  Object name_hash(&scope, strHash(thread, *name));
  return moduleValueCellAtPut(thread, module, name, name_hash, value);
}

RawObject moduleAtPutById(Thread* thread, const Module& module, SymbolId id,
                          const Object& value) {
  HandleScope scope(thread);
  Str name(&scope, thread->runtime()->symbols()->at(id));
  Object name_hash(&scope, strHash(thread, *name));
  return moduleValueCellAtPut(thread, module, name, name_hash, value);
}

RawObject moduleKeys(Thread* thread, const Module& module) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Dict module_dict(&scope, module.dict());
  Tuple buckets(&scope, module_dict.data());
  List result(&scope, runtime->newList());
  Object value(&scope, NoneType::object());
  for (word i = Dict::Bucket::kFirst; nextModuleDictItem(*buckets, &i);) {
    value = Dict::Bucket::key(*buckets, i);
    runtime->listAdd(thread, result, value);
  }
  return *result;
}

RawObject moduleLen(Thread* thread, const Module& module) {
  HandleScope scope(thread);
  Dict module_dict(&scope, module.dict());
  Tuple buckets(&scope, module_dict.data());
  word count = 0;
  for (word i = Dict::Bucket::kFirst; nextModuleDictItem(*buckets, &i);) {
    ++count;
  }
  return SmallInt::fromWord(count);
}

RawObject moduleRemove(Thread* thread, const Module& module, const Object& key,
                       const Object& key_hash) {
  HandleScope scope(thread);
  Dict module_dict(&scope, module.dict());
  Object result(&scope, thread->runtime()->dictRemove(thread, module_dict, key,
                                                      key_hash));
  DCHECK(result.isErrorNotFound() || result.isValueCell(),
         "dictRemove must return either ErrorNotFound or ValueCell");
  if (result.isErrorNotFound()) {
    return *result;
  }
  ValueCell value_cell(&scope, *result);
  icInvalidateGlobalVar(thread, value_cell);
  return value_cell.value();
}

RawObject moduleValues(Thread* thread, const Module& module) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Tuple buckets(&scope, Dict::cast(module.dict()).data());
  List result(&scope, runtime->newList());
  Object value(&scope, NoneType::object());
  for (word i = Dict::Bucket::kFirst; nextModuleDictItem(*buckets, &i);) {
    value = ValueCell::cast(Dict::Bucket::value(*buckets, i)).value();
    runtime->listAdd(thread, result, value);
  }
  return *result;
}

RawObject moduleGetAttribute(Thread* thread, const Module& module,
                             const Object& name_str, const Object& name_hash) {
  // Note that PEP 562 adds support for data descriptors in module objects.
  // We are targeting python 3.6 for now, so we won't worry about that.

  HandleScope scope(thread);
  Object result(&scope, moduleAt(thread, module, name_str, name_hash));
  if (!result.isError()) return *result;

  // TODO(T42983855) dispatching to objectGetAttribute like this does not make
  // data properties on the type override module members.

  return objectGetAttribute(thread, module, name_str, name_hash);
}

RawObject moduleSetAttr(Thread* thread, const Module& module,
                        const Object& name_str, const Object& name_hash,
                        const Object& value) {
  Runtime* runtime = thread->runtime();
  DCHECK(runtime->isInstanceOfStr(*name_str), "name must be a string");
  moduleAtPut(thread, module, name_str, name_hash, value);
  return NoneType::object();
}

bool nextModuleDictItem(RawTuple data, word* idx) {
  // Iterate through until we find a non-placeholder item.
  while (Dict::Bucket::nextItem(data, idx)) {
    if (!ValueCell::cast(Dict::Bucket::value(data, *idx)).isPlaceholder()) {
      // At this point, we have found a valid index in the buckets.
      return true;
    }
  }
  return false;
}

int execDef(Thread* thread, const Module& module, PyModuleDef* def) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Object name_obj(&scope, moduleAtById(thread, module, SymbolId::kDunderName));
  if (!runtime->isInstanceOfStr(*name_obj)) {
    thread->raiseWithFmt(LayoutId::kSystemError, "nameless module");
    return -1;
  }

  ApiHandle* handle = ApiHandle::borrowedReference(thread, *module);
  if (def->m_size >= 0) {
    if (handle->cache() == nullptr) {
      handle->setCache(std::calloc(def->m_size, 1));
      if (!handle->cache()) {
        thread->raiseMemoryError();
        return -1;
      }
    }
  }

  if (def->m_slots == nullptr) {
    return 0;
  }

  Str name_str(&scope, *name_obj);
  for (PyModuleDef_Slot* cur_slot = def->m_slots;
       cur_slot != nullptr && cur_slot->slot != 0; cur_slot++) {
    switch (cur_slot->slot) {
      case Py_mod_create:
        break;
      case Py_mod_exec: {
        typedef int (*slot_func)(PyObject*);
        slot_func thunk = reinterpret_cast<slot_func>(cur_slot->value);
        if ((*thunk)(handle) != 0) {
          if (!thread->hasPendingException()) {
            thread->raiseWithFmt(
                LayoutId::kSystemError,
                "execution of module %S failed without setting an exception",
                &name_str);
          }
          return -1;
        }
        if (thread->hasPendingException()) {
          thread->clearPendingException();
          thread->raiseWithFmt(
              LayoutId::kSystemError,
              "execution of module %S failed without setting an exception",
              &name_str);
          return -1;
        }
        break;
      }
      default: {
        thread->raiseWithFmt(LayoutId::kSystemError,
                             "module %S initialized with unknown slot %d",
                             &name_str, cur_slot->slot);
        return -1;
      }
    }
  }
  return 0;
}

RawObject moduleInit(Thread* thread, const Module& module, const Object& name) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  module.setModuleProxy(runtime->newModuleProxy(module));
  if (name.isStr()) {
    module.setName(*name);
  }
  module.setDef(runtime->newIntFromCPtr(nullptr));
  moduleAtPutById(thread, module, SymbolId::kDunderName, name);

  Object none(&scope, NoneType::object());
  moduleAtPutById(thread, module, SymbolId::kDunderDoc, none);
  moduleAtPutById(thread, module, SymbolId::kDunderPackage, none);
  moduleAtPutById(thread, module, SymbolId::kDunderLoader, none);
  moduleAtPutById(thread, module, SymbolId::kDunderSpec, none);
  return NoneType::object();
}

const BuiltinAttribute ModuleBuiltins::kAttributes[] = {
    {SymbolId::kInvalid, RawModule::kDefOffset},
    {SymbolId::kInvalid, RawModule::kDictOffset},
    {SymbolId::kInvalid, RawModule::kModuleProxyOffset},
    {SymbolId::kInvalid, RawModule::kNameOffset},
    {SymbolId::kSentinelId, -1},
};

const BuiltinMethod ModuleBuiltins::kBuiltinMethods[] = {
    {SymbolId::kDunderGetattribute, dunderGetattribute},
    {SymbolId::kDunderInit, dunderInit},
    {SymbolId::kDunderNew, dunderNew},
    {SymbolId::kDunderSetattr, dunderSetattr},
    {SymbolId::kSentinelId, nullptr},
};

RawObject ModuleBuiltins::dunderGetattribute(Thread* thread, Frame* frame,
                                             word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfModule(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kModule);
  }
  Module self(&scope, *self_obj);
  Object name(&scope, args.get(1));
  if (!runtime->isInstanceOfStr(*name)) {
    return thread->raiseWithFmt(
        LayoutId::kTypeError, "attribute name must be string, not '%T'", &name);
  }
  Object name_hash(&scope, Interpreter::hash(thread, name));
  if (name_hash.isErrorException()) return *name_hash;
  Object result(&scope, moduleGetAttribute(thread, self, name, name_hash));
  if (result.isErrorNotFound()) {
    Object module_name(&scope, self.name());
    if (!runtime->isInstanceOfStr(*module_name)) {
      return thread->raiseWithFmt(LayoutId::kAttributeError,
                                  "module has no attribute '%S'", &name);
    }
    return thread->raiseWithFmt(LayoutId::kAttributeError,
                                "module '%S' has no attribute '%S'",
                                &module_name, &name);
  }
  return *result;
}

RawObject ModuleBuiltins::dunderNew(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object cls_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfType(*cls_obj)) {
    return thread->raiseWithFmt(
        LayoutId::kTypeError, "module.__new__(X): X is not a type object (%T)",
        &cls_obj);
  }
  Type cls(&scope, *cls_obj);
  if (cls.builtinBase() != LayoutId::kModule) {
    Object cls_name(&scope, cls.name());
    return thread->raiseWithFmt(
        LayoutId::kTypeError,
        "module.__new__(%S): %S is not a subtype of module", &cls_name,
        &cls_name);
  }
  Layout layout(&scope, cls.instanceLayout());
  Module result(&scope, runtime->newInstance(layout));
  // Note that this is different from cpython, which initializes `__dict__` to
  // `None` and only sets it in `module.__init__()`. Setting a dictionary right
  // has the having a dictionary there becomes an invariant, because the field
  // is read-only otherwise, so we can generally skip type tests for it.
  result.setDict(runtime->newDict());
  result.setDef(runtime->newIntFromCPtr(nullptr));
  return *result;
}

RawObject ModuleBuiltins::dunderInit(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfModule(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kModule);
  }
  Module self(&scope, *self_obj);
  Object name(&scope, args.get(1));
  if (!runtime->isInstanceOfStr(*name)) {
    return thread->raiseWithFmt(
        LayoutId::kTypeError,
        "module.__init__() argument 1 must be str, not %T", &name);
  }
  return moduleInit(thread, self, name);
}

RawObject ModuleBuiltins::dunderSetattr(Thread* thread, Frame* frame,
                                        word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfModule(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kModule);
  }
  Module self(&scope, *self_obj);
  Object name(&scope, args.get(1));
  if (!runtime->isInstanceOfStr(*name)) {
    return thread->raiseWithFmt(
        LayoutId::kTypeError, "attribute name must be string, not '%T'", &name);
  }
  Object name_hash(&scope, Interpreter::hash(thread, name));
  if (name_hash.isErrorException()) return *name_hash;
  Object value(&scope, args.get(2));
  return moduleSetAttr(thread, self, name, name_hash, value);
}

}  // namespace py

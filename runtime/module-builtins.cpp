#include "module-builtins.h"

#include "frame.h"
#include "globals.h"
#include "list-builtins.h"
#include "object-builtins.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace python {

RawObject moduleAt(Thread* thread, const Module& module, const Object& key) {
  HandleScope scope(thread);
  Dict dict(&scope, module.dict());
  return moduleDictAt(thread, dict, key);
}

RawObject moduleAtById(Thread* thread, const Module& module, SymbolId key) {
  HandleScope scope(thread);
  Object key_obj(&scope, thread->runtime()->symbols()->at(key));
  return moduleAt(thread, module, key_obj);
}

RawObject moduleDictAt(Thread* thread, const Dict& module_dict,
                       const Object& key) {
  RawObject result = moduleDictValueCellAt(thread, module_dict, key);
  if (!result.isErrorNotFound()) {
    return ValueCell::cast(result).value();
  }
  return Error::notFound();
}

RawObject moduleDictValueCellAt(Thread* thread, const Dict& dict,
                                const Object& key) {
  HandleScope scope(thread);
  Object result(&scope, thread->runtime()->dictAt(thread, dict, key));
  DCHECK(result.isErrorNotFound() || result.isValueCell(),
         "global dict lookup result must return either ErrorNotFound or "
         "ValueCell");
  if (result.isErrorNotFound() || ValueCell::cast(*result).isPlaceholder()) {
    return Error::notFound();
  }
  return *result;
}

RawDict moduleDictBuiltins(Thread* thread, const Dict& dict) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object dunder_builtins_name(&scope, runtime->symbols()->DunderBuiltins());
  Object builtins_module(&scope,
                         moduleDictAt(thread, dict, dunder_builtins_name));
  if (!builtins_module.isErrorNotFound()) {
    CHECK(builtins_module.isModule(), "expected builtins module");
    return RawDict::cast(Module::cast(*builtins_module).dict());
  }

  // Create a minimal builtins dictionary with just `{'None': None}`.
  Dict builtins(&scope, runtime->newDict());
  Object none_name(&scope, runtime->symbols()->None());
  Object none(&scope, NoneType::object());
  moduleDictAtPut(thread, builtins, none_name, none);
  return *builtins;
}

RawObject moduleAtPut(Thread* thread, const Module& module, const Object& key,
                      const Object& value) {
  HandleScope scope(thread);
  Dict dict(&scope, module.dict());
  // TODO(T42983855) Respect data properties here.
  return moduleDictAtPut(thread, dict, key, value);
}

RawObject moduleAtPutById(Thread* thread, const Module& module, SymbolId key,
                          const Object& value) {
  HandleScope scope(thread);
  Dict dict(&scope, module.dict());
  Object key_name(&scope, thread->runtime()->symbols()->at(key));
  return moduleDictAtPut(thread, dict, key_name, value);
}

RawObject moduleDictAtPut(Thread* thread, const Dict& module_dict,
                          const Object& key, const Object& value) {
  HandleScope scope(thread);
  Object result(&scope,
                moduleDictValueCellAtPut(thread, module_dict, key, value));
  if (result.isError()) {
    return *result;
  }
  return ValueCell::cast(*result).value();
}

RawObject moduleDictValueCellAtPut(Thread* thread, const Dict& module_dict,
                                   const Object& key, const Object& value) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object module_result(&scope, runtime->dictAt(thread, module_dict, key));
  if (module_result.isValueCell() &&
      ValueCell::cast(*module_result).isPlaceholder()) {
    // A builtin entry is cached under the same key, so invalidate its caches.
    Dict builtins(&scope, moduleDictBuiltins(thread, module_dict));
    Object builtins_result(&scope,
                           moduleDictValueCellAt(thread, builtins, key));
    DCHECK(builtins_result.isValueCell(), "a builtin entry must exist");
    ValueCell builtins_value_cell(&scope, *builtins_result);
    DCHECK(!builtins_value_cell.dependencyLink().isNoneType(),
           "the builtin valuecell must have a dependent");
    icInvalidateGlobalVar(thread, builtins_value_cell);
  }
  return thread->runtime()->dictAtPutInValueCell(thread, module_dict, key,
                                                 value);
}

RawObject moduleDictRemove(Thread* thread, const Dict& module_dict,
                           const Object& key) {
  HandleScope scope(thread);
  Object result(&scope,
                thread->runtime()->dictRemove(thread, module_dict, key));
  DCHECK(result.isErrorNotFound() || result.isValueCell(),
         "dictRemove must return either ErrorNotFound or ValueCell");
  if (result.isErrorNotFound()) {
    return *result;
  }
  ValueCell value_cell(&scope, *result);
  icInvalidateGlobalVar(thread, value_cell);
  return value_cell.value();
}

RawObject moduleDictKeys(Thread* thread, const Dict& module_dict) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
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
                             const Object& name_str) {
  // Note that PEP 562 adds support for data descriptors in module objects.
  // We are targeting python 3.6 for now, so we won't worry about that.

  HandleScope scope(thread);
  Object result(&scope, moduleAt(thread, module, name_str));
  if (!result.isError()) return *result;

  // TODO(T42983855) dispatching to objectGetAttribute like this does not make
  // data properties on the type override module members.

  return objectGetAttribute(thread, module, name_str);
}

RawObject moduleSetAttr(Thread* thread, const Module& module,
                        const Object& name_str, const Object& value) {
  Runtime* runtime = thread->runtime();
  DCHECK(runtime->isInstanceOfStr(*name_str), "name must be a string");
  moduleAtPut(thread, module, name_str, value);
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
  Str key(&scope, runtime->symbols()->DunderName());
  Object name_obj(&scope, moduleAt(thread, module, key));
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

const BuiltinAttribute ModuleBuiltins::kAttributes[] = {
    {SymbolId::kInvalid, RawModule::kDictOffset},
    {SymbolId::kInvalid, RawModule::kModuleProxyOffset},
    {SymbolId::kInvalid, RawModule::kNameOffset},
    {SymbolId::kSentinelId, -1},
};

const BuiltinMethod ModuleBuiltins::kBuiltinMethods[] = {
    {SymbolId::kDunderGetattribute, dunderGetattribute},
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
  Object result(&scope, moduleGetAttribute(thread, self, name));
  if (result.isErrorNotFound()) {
    Object module_name(&scope, self.name());
    return thread->raiseWithFmt(LayoutId::kAttributeError,
                                "module '%S' has no attribute '%S'",
                                &module_name, &name);
  }
  return *result;
}

RawObject ModuleBuiltins::dunderNew(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  if (!args.get(0).isType()) {
    return thread->raiseWithFmt(LayoutId::kTypeError, "not a type object");
  }
  if (Type::cast(args.get(0)).builtinBase() != LayoutId::kModule) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "not a subtype of module");
  }
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfStr(args.get(1))) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "argument must be a str instance");
  }
  HandleScope scope(thread);
  Str name(&scope, args.get(1));
  return runtime->newModule(name);
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
  if (!name.isStr()) {
    UNIMPLEMENTED("Strict subclass of string");
  }
  Object value(&scope, args.get(2));
  return moduleSetAttr(thread, self, name, value);
}

}  // namespace python

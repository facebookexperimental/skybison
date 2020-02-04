#include "module-builtins.h"

#include "builtins.h"
#include "capi-handles.h"
#include "dict-builtins.h"
#include "frame.h"
#include "globals.h"
#include "ic.h"
#include "object-builtins.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"
#include "type-builtins.h"

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

RawObject moduleAt(Thread* thread, const Module& module, const Object& name) {
  HandleScope scope(thread);
  Dict dict(&scope, module.dict());
  return unwrapValueCell(dictAtByStr(thread, dict, name));
}

RawObject moduleAtById(Thread* thread, const Module& module, SymbolId id) {
  HandleScope scope(thread);
  Dict dict(&scope, module.dict());
  return unwrapValueCell(dictAtById(thread, dict, id));
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
  return filterPlaceholderValueCell(dictAtById(thread, dict, id));
}

RawObject moduleValueCellAt(Thread* thread, const Module& module,
                            const Object& name) {
  HandleScope scope(thread);
  Dict dict(&scope, module.dict());
  return filterPlaceholderValueCell(dictAtByStr(thread, dict, name));
}

static RawObject moduleValueCellAtPut(Thread* thread, const Module& module,
                                      const Object& name, const Object& value) {
  HandleScope scope(thread);
  Dict module_dict(&scope, module.dict());
  Object module_result(&scope, dictAtByStr(thread, module_dict, name));
  if (module_result.isValueCell() &&
      ValueCell::cast(*module_result).isPlaceholder()) {
    // A builtin entry is cached under the same name, so invalidate its caches.
    Module builtins_module(&scope,
                           moduleAtById(thread, module, ID(__builtins__)));
    Dict builtins_dict(&scope, builtins_module.dict());
    Object builtins_result(&scope, filterPlaceholderValueCell(dictAtByStr(
                                       thread, builtins_dict, name)));
    DCHECK(builtins_result.isValueCell(), "a builtin entry must exist");
    ValueCell builtins_value_cell(&scope, *builtins_result);
    DCHECK(!builtins_value_cell.dependencyLink().isNoneType(),
           "the builtin valuecell must have a dependent");
    icInvalidateGlobalVar(thread, builtins_value_cell);
  }
  return dictAtPutInValueCellByStr(thread, module_dict, name, value);
}

RawObject moduleAtPut(Thread* thread, const Module& module, const Object& name,
                      const Object& value) {
  return moduleValueCellAtPut(thread, module, name, value);
}

RawObject moduleAtPutById(Thread* thread, const Module& module, SymbolId id,
                          const Object& value) {
  HandleScope scope(thread);
  Object name(&scope, thread->runtime()->symbols()->at(id));
  return moduleValueCellAtPut(thread, module, name, value);
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

RawObject moduleRaiseAttributeError(Thread* thread, const Module& module,
                                    const Object& name) {
  HandleScope scope(thread);
  Object module_name(&scope, module.name());
  if (!thread->runtime()->isInstanceOfStr(*module_name)) {
    return thread->raiseWithFmt(LayoutId::kAttributeError,
                                "module has no attribute '%S'", &name);
  }
  return thread->raiseWithFmt(LayoutId::kAttributeError,
                              "module '%S' has no attribute '%S'", &module_name,
                              &name);
}

RawObject moduleRemove(Thread* thread, const Module& module, const Object& key,
                       word hash) {
  HandleScope scope(thread);
  Dict module_dict(&scope, module.dict());
  Object result(&scope, dictRemove(thread, module_dict, key, hash));
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
                             const Object& name) {
  return moduleGetAttributeSetLocation(thread, module, name, nullptr);
}

RawObject moduleGetAttributeSetLocation(Thread* thread, const Module& module,
                                        const Object& name,
                                        Object* location_out) {
  // Note that PEP 562 adds support for data descriptors in module objects.
  // We are targeting python 3.6 for now, so we won't worry about that.

  HandleScope scope(thread);
  Object result(&scope, moduleValueCellAt(thread, module, name));

  DCHECK(result.isValueCell() || result.isErrorNotFound(),
         "result must be a value cell or not found");

  if (!result.isErrorNotFound() && !ValueCell::cast(*result).isPlaceholder()) {
    if (location_out != nullptr) {
      *location_out = *result;
    }
    return ValueCell::cast(*result).value();
  }

  // TODO(T42983855) dispatching to objectGetAttribute like this does not make
  // data properties on the type override module members.

  return objectGetAttribute(thread, module, name);
}

RawObject moduleSetAttr(Thread* thread, const Module& module,
                        const Object& name, const Object& value) {
  moduleAtPut(thread, module, name, value);
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
  Object name_obj(&scope, moduleAtById(thread, module, ID(__name__)));
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
  moduleAtPutById(thread, module, ID(__name__), name);

  Object none(&scope, NoneType::object());
  moduleAtPutById(thread, module, ID(__doc__), none);
  moduleAtPutById(thread, module, ID(__package__), none);
  moduleAtPutById(thread, module, ID(__loader__), none);
  moduleAtPutById(thread, module, ID(__spec__), none);
  return NoneType::object();
}

const BuiltinAttribute ModuleBuiltins::kAttributes[] = {
    {ID(_module__def), RawModule::kDefOffset, AttributeFlags::kHidden},
    {ID(_module__dict), RawModule::kDictOffset, AttributeFlags::kHidden},
    {ID(_module__proxy), RawModule::kModuleProxyOffset,
     AttributeFlags::kHidden},
    {ID(_module__name), RawModule::kNameOffset, AttributeFlags::kHidden},
    {SymbolId::kSentinelId, -1},
};

RawObject METH(module, __getattribute__)(Thread* thread, Frame* frame,
                                         word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfModule(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(module));
  }
  Module self(&scope, *self_obj);
  Object name(&scope, args.get(1));
  name = attributeName(thread, name);
  if (name.isErrorException()) return *name;
  Object result(&scope, moduleGetAttribute(thread, self, name));
  if (result.isErrorNotFound()) {
    return moduleRaiseAttributeError(thread, self, name);
  }
  return *result;
}

RawObject METH(module, __new__)(Thread* thread, Frame* frame, word nargs) {
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
  result.setId(runtime->reserveModuleId());
  return *result;
}

RawObject METH(module, __init__)(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfModule(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(module));
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

RawObject METH(module, __setattr__)(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfModule(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(module));
  }
  Module self(&scope, *self_obj);
  Object name(&scope, args.get(1));
  name = attributeName(thread, name);
  if (name.isErrorException()) return *name;
  Object value(&scope, args.get(2));
  return moduleSetAttr(thread, self, name, value);
}

}  // namespace py

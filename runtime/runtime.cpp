#include "runtime.h"

#include <unistd.h>
#include <climits>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <memory>

#include "builtins-module.h"
#include "bytecode.h"
#include "callback.h"
#include "complex-builtins.h"
#include "descriptor-builtins.h"
#include "dict-builtins.h"
#include "exception-builtins.h"
#include "float-builtins.h"
#include "frame.h"
#include "frozen-modules.h"
#include "function-builtins.h"
#include "globals.h"
#include "handles.h"
#include "heap.h"
#include "int-builtins.h"
#include "interpreter.h"
#include "layout.h"
#include "list-builtins.h"
#include "marshal.h"
#include "object-builtins.h"
#include "os.h"
#include "range-builtins.h"
#include "ref-builtins.h"
#include "scavenger.h"
#include "set-builtins.h"
#include "siphash.h"
#include "str-builtins.h"
#include "super-builtins.h"
#include "sys-module.h"
#include "thread.h"
#include "time-module.h"
#include "trampolines-inl.h"
#include "tuple-builtins.h"
#include "type-builtins.h"
#include "utils.h"
#include "visitor.h"

namespace python {

static const SymbolId kBinaryOperationSelector[] = {
    SymbolId::kDunderAdd,     SymbolId::kDunderSub,
    SymbolId::kDunderMul,     SymbolId::kDunderMatmul,
    SymbolId::kDunderTruediv, SymbolId::kDunderFloordiv,
    SymbolId::kDunderMod,     SymbolId::kDunderDivmod,
    SymbolId::kDunderPow,     SymbolId::kDunderLshift,
    SymbolId::kDunderRshift,  SymbolId::kDunderAnd,
    SymbolId::kDunderXor,     SymbolId::kDunderOr};

static const SymbolId kSwappedBinaryOperationSelector[] = {
    SymbolId::kDunderRadd,     SymbolId::kDunderRsub,
    SymbolId::kDunderRmul,     SymbolId::kDunderRmatmul,
    SymbolId::kDunderRtruediv, SymbolId::kDunderRfloordiv,
    SymbolId::kDunderRmod,     SymbolId::kDunderRdivmod,
    SymbolId::kDunderRpow,     SymbolId::kDunderRlshift,
    SymbolId::kDunderRrshift,  SymbolId::kDunderRand,
    SymbolId::kDunderRxor,     SymbolId::kDunderRor};

static const SymbolId kInplaceOperationSelector[] = {
    SymbolId::kDunderIadd,     SymbolId::kDunderIsub,
    SymbolId::kDunderImul,     SymbolId::kDunderImatmul,
    SymbolId::kDunderItruediv, SymbolId::kDunderIfloordiv,
    SymbolId::kDunderImod,     SymbolId::kMaxId,
    SymbolId::kDunderIpow,     SymbolId::kDunderIlshift,
    SymbolId::kDunderIrshift,  SymbolId::kDunderIand,
    SymbolId::kDunderIxor,     SymbolId::kDunderIor};

static const SymbolId kComparisonSelector[] = {
    SymbolId::kDunderLt, SymbolId::kDunderLe, SymbolId::kDunderEq,
    SymbolId::kDunderNe, SymbolId::kDunderGt, SymbolId::kDunderGe};

Runtime::Runtime(word heap_size)
    : heap_(heap_size), new_value_cell_callback_(this) {
  initializeRandom();
  initializeThreads();
  // This must be called before initializeClasses is called. Methods in
  // initializeClasses rely on instances that are created in this method.
  initializePrimitiveInstances();
  initializeInterned();
  initializeSymbols();
  initializeClasses();
  initializeModules();
  initializeApiHandles();
}

Runtime::Runtime() : Runtime(64 * kMiB) {}

Runtime::~Runtime() {
  // TODO(T30392425): This is an ugly and fragile workaround for having multiple
  // runtimes created and destroyed by a single thread.
  if (Thread::currentThread() == nullptr) {
    CHECK(threads_ != nullptr, "the runtime does not have any threads");
    Thread::setCurrentThread(threads_);
  }
  freeApiHandles();
  for (Thread* thread = threads_; thread != nullptr;) {
    if (thread == Thread::currentThread()) {
      Thread::setCurrentThread(nullptr);
    } else {
      UNIMPLEMENTED("threading");
    }
    auto prev = thread;
    thread = thread->next();
    delete prev;
  }
  threads_ = nullptr;
  delete symbols_;
}

Object* Runtime::newBoundMethod(const Handle<Object>& function,
                                const Handle<Object>& self) {
  HandleScope scope;
  Handle<BoundMethod> bound_method(&scope, heap()->createBoundMethod());
  bound_method->setFunction(*function);
  bound_method->setSelf(*self);
  return *bound_method;
}

Object* Runtime::newLayout() {
  HandleScope scope;
  Handle<Layout> layout(&scope, heap()->createLayout(LayoutId::kError));
  layout->setInObjectAttributes(empty_object_array_);
  layout->setOverflowAttributes(empty_object_array_);
  layout->setAdditions(newList());
  layout->setDeletions(newList());
  layout->setNumInObjectAttributes(0);
  return *layout;
}

Object* Runtime::layoutCreateSubclassWithBuiltins(
    LayoutId subclass_id, LayoutId superclass_id,
    View<BuiltinAttribute> attributes) {
  HandleScope scope;

  // A builtin class is special since it contains attributes that must be
  // located at fixed offsets from the start of an instance.  These attributes
  // are packed at the beginning of the layout starting at offset 0.
  Handle<Layout> super_layout(&scope, layoutAt(superclass_id));
  Handle<ObjectArray> super_attributes(&scope,
                                       super_layout->inObjectAttributes());

  // Sanity check that a subclass that has fixed attributes does inherit from a
  // superclass with attributes that are not fixed.
  for (word i = 0; i < super_attributes->length(); i++) {
    Handle<ObjectArray> elt(&scope, super_attributes->at(i));
    AttributeInfo info(elt->at(1));
    CHECK(info.isInObject() && info.isFixedOffset(),
          "all superclass attributes must be in-object and fixed");
  }

  // Create an empty layout for the subclass
  Handle<Layout> result(&scope, newLayout());
  result->setId(subclass_id);

  // Copy down all of the superclass attributes into the subclass layout
  Handle<ObjectArray> in_object(
      &scope, newObjectArray(super_attributes->length() + attributes.length()));
  super_attributes->copyTo(*in_object);
  appendBuiltinAttributes(attributes, in_object, super_attributes->length());

  // Install the in-object attributes
  result->setInObjectAttributes(*in_object);
  result->setNumInObjectAttributes(in_object->length());

  return *result;
}

void Runtime::appendBuiltinAttributes(View<BuiltinAttribute> attributes,
                                      const Handle<ObjectArray>& dst,
                                      word start_index) {
  if (attributes.length() == 0) {
    return;
  }
  HandleScope scope;
  Handle<ObjectArray> entry(&scope, empty_object_array_);
  for (word i = 0; i < attributes.length(); i++) {
    AttributeInfo info(
        attributes.get(i).offset,
        AttributeInfo::Flag::kInObject | AttributeInfo::Flag::kFixedOffset);
    entry = newObjectArray(2);
    entry->atPut(0, symbols()->at(attributes.get(i).name));
    entry->atPut(1, info.asSmallInt());
    dst->atPut(start_index + i, *entry);
  }
}

Object* Runtime::addEmptyBuiltinClass(SymbolId name, LayoutId subclass_id,
                                      LayoutId superclass_id) {
  return addBuiltinClass(name, subclass_id, superclass_id,
                         View<BuiltinAttribute>(nullptr, 0));
}

Object* Runtime::addBuiltinClass(SymbolId name, LayoutId subclass_id,
                                 LayoutId superclass_id,
                                 View<BuiltinAttribute> attributes) {
  return addBuiltinClass(name, subclass_id, superclass_id, attributes,
                         View<BuiltinMethod>(nullptr, 0));
}

Object* Runtime::addBuiltinClass(SymbolId name, LayoutId subclass_id,
                                 LayoutId superclass_id,
                                 View<BuiltinMethod> methods) {
  return addBuiltinClass(name, subclass_id, superclass_id,
                         View<BuiltinAttribute>(nullptr, 0), methods);
}

Object* Runtime::addBuiltinClass(SymbolId name, LayoutId subclass_id,
                                 LayoutId superclass_id,
                                 View<BuiltinAttribute> attributes,
                                 View<BuiltinMethod> methods) {
  HandleScope scope;

  // Create a class object for the subclass
  Handle<Type> subclass(&scope, newClass());
  subclass->setName(symbols()->at(name));

  Handle<Layout> layout(&scope, layoutCreateSubclassWithBuiltins(
                                    subclass_id, superclass_id, attributes));

  // Assign the layout to the class
  layout->setDescribedClass(*subclass);

  // Now we can create an MRO
  Handle<ObjectArray> mro(&scope, createMro(layout, superclass_id));

  subclass->setMro(*mro);
  subclass->setInstanceLayout(*layout);
  Handle<Type> superclass(&scope, typeAt(superclass_id));
  subclass->setFlags(superclass->flags());

  // Install the layout and class
  layoutAtPut(subclass_id, *layout);

  // Add the provided methods.
  for (const BuiltinMethod& meth : methods) {
    classAddBuiltinFunction(subclass, meth.name, meth.address);
  }

  // return the class
  return *subclass;
}

Object* Runtime::newByteArray(word length, byte fill) {
  DCHECK(length >= 0, "invalid length %ld", length);
  if (length == 0) {
    return empty_byte_array_;
  }
  Object* result = heap()->createByteArray(length);
  byte* dst = reinterpret_cast<byte*>(ByteArray::cast(result)->address());
  std::memset(dst, fill, length);
  return result;
}

Object* Runtime::newByteArrayWithAll(View<byte> array) {
  if (array.length() == 0) {
    return empty_byte_array_;
  }
  Object* result = heap()->createByteArray(array.length());
  byte* dst = reinterpret_cast<byte*>(ByteArray::cast(result)->address());
  std::memcpy(dst, array.data(), array.length());
  return result;
}

Object* Runtime::newClass() { return newClassWithMetaclass(LayoutId::kType); }

Object* Runtime::newClassWithMetaclass(LayoutId metaclass_id) {
  HandleScope scope;
  Handle<Type> result(&scope, heap()->createClass(metaclass_id));
  Handle<Dict> dict(&scope, newDict());
  result->setFlags(SmallInt::fromWord(0));
  result->setDict(*dict);
  return *result;
}

Object* Runtime::classGetAttr(Thread* thread, const Handle<Object>& receiver,
                              const Handle<Object>& name) {
  if (!name->isStr()) {
    // TODO(T25140871): Refactor into something like:
    //     thread->throwUnexpectedTypeError(expected, actual)
    return thread->throwTypeErrorFromCStr("attribute name must be a string");
  }

  HandleScope scope(thread);
  Handle<Type> klass(&scope, *receiver);
  Handle<Type> meta_klass(&scope, typeOf(*receiver));

  // Look for the attribute in the meta class
  Handle<Object> meta_attr(&scope, lookupNameInMro(thread, meta_klass, name));
  if (!meta_attr->isError()) {
    if (isDataDescriptor(thread, meta_attr)) {
      // TODO(T25692531): Call __get__ from meta_attr
      UNIMPLEMENTED("custom descriptors are unsupported");
    }
  }

  // No data descriptor found on the meta class, look in the mro of the klass
  Handle<Object> attr(&scope, lookupNameInMro(thread, klass, name));
  if (!attr->isError()) {
    if (isNonDataDescriptor(thread, attr)) {
      Handle<Object> instance(&scope, None::object());
      return Interpreter::callDescriptorGet(thread, thread->currentFrame(),
                                            attr, instance, receiver);
    }
    return *attr;
  }

  // No attr found in klass or its mro, use the non-data descriptor found in
  // the metaclass (if any).
  if (!meta_attr->isError()) {
    if (isNonDataDescriptor(thread, meta_attr)) {
      Handle<Object> owner(&scope, *meta_klass);
      return Interpreter::callDescriptorGet(thread, thread->currentFrame(),
                                            meta_attr, receiver, owner);
    }

    // If a regular attribute was found in the metaclass, return it
    if (!meta_attr->isError()) {
      return *meta_attr;
    }
  }

  // TODO(T25140871): Refactor this into something like:
  //     thread->throwMissingAttributeError(name)
  return thread->throwAttributeErrorFromCStr("missing attribute");
}

Object* Runtime::classSetAttr(Thread* thread, const Handle<Object>& receiver,
                              const Handle<Object>& name,
                              const Handle<Object>& value) {
  if (!name->isStr()) {
    // TODO(T25140871): Refactor into something like:
    //     thread->throwUnexpectedTypeError(expected, actual)
    return thread->throwTypeErrorFromCStr("attribute name must be a string");
  }

  HandleScope scope(thread);
  Handle<Type> klass(&scope, *receiver);
  if (klass->isIntrinsicOrExtension()) {
    // TODO(T25140871): Refactor this into something that includes the type name
    // like:
    //     thread->throwImmutableTypeManipulationError(klass)
    return thread->throwTypeErrorFromCStr(
        "can't set attributes of built-in/extension type");
  }

  // Check for a data descriptor
  Handle<Type> metaklass(&scope, typeOf(*receiver));
  Handle<Object> meta_attr(&scope, lookupNameInMro(thread, metaklass, name));
  if (!meta_attr->isError()) {
    if (isDataDescriptor(thread, meta_attr)) {
      // TODO(T25692531): Call __set__ from meta_attr
      UNIMPLEMENTED("custom descriptors are unsupported");
    }
  }

  // No data descriptor found, store the attribute in the klass dict
  Handle<Dict> klass_dict(&scope, klass->dict());
  dictAtPutInValueCell(klass_dict, name, value);

  return None::object();
}

Object* Runtime::classDelAttr(Thread* thread, const Handle<Object>& receiver,
                              const Handle<Object>& name) {
  if (!name->isStr()) {
    // TODO(T25140871): Refactor into something like:
    //     thread->throwUnexpectedTypeError(expected, actual)
    return thread->throwTypeErrorFromCStr("attribute name must be a string");
  }

  HandleScope scope(thread);
  Handle<Type> klass(&scope, *receiver);
  // TODO(mpage): This needs to handle built-in extension types.
  if (klass->isIntrinsicOrExtension()) {
    // TODO(T25140871): Refactor this into something that includes the type name
    // like:
    //     thread->throwImmutableTypeManipulationError(klass)
    return thread->throwTypeErrorFromCStr(
        "can't set attributes of built-in/extension type");
  }

  // Check for a delete descriptor
  Handle<Type> metaklass(&scope, typeOf(*receiver));
  Handle<Object> meta_attr(&scope, lookupNameInMro(thread, metaklass, name));
  if (!meta_attr->isError()) {
    if (isDeleteDescriptor(thread, meta_attr)) {
      return Interpreter::callDescriptorDelete(thread, thread->currentFrame(),
                                               meta_attr, receiver);
    }
  }

  // No delete descriptor found, attempt to delete from the klass dict
  Handle<Dict> klass_dict(&scope, klass->dict());
  if (!dictRemove(klass_dict, name, nullptr)) {
    // TODO(T25140871): Refactor this into something like:
    //     thread->throwMissingAttributeError(name)
    return thread->throwAttributeErrorFromCStr("missing attribute");
  }

  return None::object();
}

// Generic attribute lookup code used for instance objects
Object* Runtime::instanceGetAttr(Thread* thread, const Handle<Object>& receiver,
                                 const Handle<Object>& name) {
  if (!name->isStr()) {
    // TODO(T25140871): Refactor into something like:
    //     thread->throwUnexpectedTypeError(expected, actual)
    return thread->throwTypeErrorFromCStr("attribute name must be a string");
  }

  if (Str::cast(*name)->equals(symbols()->DunderClass())) {
    // TODO(T27735822): Make __class__ a descriptor
    return typeOf(*receiver);
  }

  // Look for the attribute in the class
  HandleScope scope(thread);
  Handle<Type> klass(&scope, typeOf(*receiver));
  Handle<Object> klass_attr(&scope, lookupNameInMro(thread, klass, name));
  if (!klass_attr->isError()) {
    if (isDataDescriptor(thread, klass_attr)) {
      Handle<Object> owner(&scope, *klass);
      return Interpreter::callDescriptorGet(thread, thread->currentFrame(),
                                            klass_attr, receiver, owner);
    }
  }

  // No data descriptor found on the class, look at the instance.
  if (receiver->isHeapObject()) {
    Handle<HeapObject> instance(&scope, *receiver);
    Object* result = thread->runtime()->instanceAt(thread, instance, name);
    if (!result->isError()) {
      return result;
    }
  }

  // Nothing found in the instance, if we found a non-data descriptor via the
  // class search, use it.
  if (!klass_attr->isError()) {
    if (isNonDataDescriptor(thread, klass_attr)) {
      Handle<Object> owner(&scope, *klass);
      return Interpreter::callDescriptorGet(thread, thread->currentFrame(),
                                            klass_attr, receiver, owner);
    }

    // If a regular attribute was found in the class, return it
    return *klass_attr;
  }

  // TODO(T25140871): Refactor this into something like:
  //     thread->throwMissingAttributeError(name)
  return thread->throwAttributeErrorFromCStr("missing attribute");
}

Object* Runtime::instanceSetAttr(Thread* thread, const Handle<Object>& receiver,
                                 const Handle<Object>& name,
                                 const Handle<Object>& value) {
  if (!name->isStr()) {
    // TODO(T25140871): Refactor into something like:
    //     thread->throwUnexpectedTypeError(expected, actual)
    return thread->throwTypeErrorFromCStr("attribute name must be a string");
  }

  // Check for a data descriptor
  HandleScope scope(thread);
  Handle<Type> klass(&scope, typeOf(*receiver));
  Handle<Object> klass_attr(&scope, lookupNameInMro(thread, klass, name));
  if (!klass_attr->isError()) {
    if (isDataDescriptor(thread, klass_attr)) {
      return Interpreter::callDescriptorSet(thread, thread->currentFrame(),
                                            klass_attr, receiver, value);
    }
  }

  // No data descriptor found, store on the instance
  Handle<HeapObject> instance(&scope, *receiver);
  return thread->runtime()->instanceAtPut(thread, instance, name, value);
}

Object* Runtime::instanceDelAttr(Thread* thread, const Handle<Object>& receiver,
                                 const Handle<Object>& name) {
  if (!name->isStr()) {
    // TODO(T25140871): Refactor into something like:
    //     thread->throwUnexpectedTypeError(expected, actual)
    return thread->throwTypeErrorFromCStr("attribute name must be a string");
  }

  // Check for a descriptor with __delete__
  HandleScope scope(thread);
  Handle<Type> klass(&scope, typeOf(*receiver));
  Handle<Object> klass_attr(&scope, lookupNameInMro(thread, klass, name));
  if (!klass_attr->isError()) {
    if (isDeleteDescriptor(thread, klass_attr)) {
      return Interpreter::callDescriptorDelete(thread, thread->currentFrame(),
                                               klass_attr, receiver);
    }
  }

  // No delete descriptor found, delete from the instance
  Handle<HeapObject> instance(&scope, *receiver);
  Handle<Object> result(&scope, instanceDel(thread, instance, name));
  if (result->isError()) {
    // TODO(T25140871): Refactor this into something like:
    //     thread->throwMissingAttributeError(name)
    return thread->throwAttributeErrorFromCStr("missing attribute");
  }

  return *result;
}

// Note that PEP 562 adds support for data descriptors in module objects.
// We are targeting python 3.6 for now, so we won't worry about that.
Object* Runtime::moduleGetAttr(Thread* thread, const Handle<Object>& receiver,
                               const Handle<Object>& name) {
  if (!name->isStr()) {
    // TODO(T25140871): Refactor into something like:
    //     thread->throwUnexpectedTypeError(expected, actual)
    return thread->throwTypeErrorFromCStr("attribute name must be a string");
  }

  HandleScope scope(thread);
  Handle<Module> mod(&scope, *receiver);
  Handle<Object> ret(&scope, moduleAt(mod, name));

  if (!ret->isError()) {
    return *ret;
  } else {
    // TODO(T25140871): Refactor this into something like:
    //     thread->throwMissingAttributeError(name)
    return thread->throwAttributeErrorFromCStr("missing attribute");
  }
}

Object* Runtime::moduleSetAttr(Thread* thread, const Handle<Object>& receiver,
                               const Handle<Object>& name,
                               const Handle<Object>& value) {
  if (!name->isStr()) {
    // TODO(T25140871): Refactor into something like:
    //     thread->throwUnexpectedTypeError(expected, actual)
    return thread->throwTypeErrorFromCStr("attribute name must be a string");
  }

  HandleScope scope(thread);
  Handle<Module> mod(&scope, *receiver);
  moduleAtPut(mod, name, value);
  return None::object();
}

Object* Runtime::moduleDelAttr(Thread* thread, const Handle<Object>& receiver,
                               const Handle<Object>& name) {
  if (!name->isStr()) {
    // TODO(T25140871): Refactor into something like:
    //     thread->throwUnexpectedTypeError(expected, actual)
    return thread->throwTypeErrorFromCStr("attribute name must be a string");
  }

  // Check for a descriptor with __delete__
  HandleScope scope(thread);
  Handle<Type> klass(&scope, typeOf(*receiver));
  Handle<Object> klass_attr(&scope, lookupNameInMro(thread, klass, name));
  if (!klass_attr->isError()) {
    if (isDeleteDescriptor(thread, klass_attr)) {
      return Interpreter::callDescriptorDelete(thread, thread->currentFrame(),
                                               klass_attr, receiver);
    }
  }

  // No delete descriptor found, attempt to delete from the module dict
  Handle<Module> module(&scope, *receiver);
  Handle<Dict> module_dict(&scope, module->dict());
  if (!dictRemove(module_dict, name, nullptr)) {
    // TODO(T25140871): Refactor this into something like:
    //     thread->throwMissingAttributeError(name)
    return thread->throwAttributeErrorFromCStr("missing attribute");
  }

  return None::object();
}

bool Runtime::isDataDescriptor(Thread* thread, const Handle<Object>& object) {
  // TODO(T25692962): Track "descriptorness" through a bit on the class
  HandleScope scope(thread);
  Handle<Type> type(&scope, typeOf(*object));
  return !lookupSymbolInMro(thread, type, SymbolId::kDunderSet)->isError();
}

bool Runtime::isNonDataDescriptor(Thread* thread,
                                  const Handle<Object>& object) {
  // TODO(T25692962): Track "descriptorness" through a bit on the class
  HandleScope scope(thread);
  Handle<Type> type(&scope, typeOf(*object));
  return !lookupSymbolInMro(thread, type, SymbolId::kDunderGet)->isError();
}

bool Runtime::isDeleteDescriptor(Thread* thread, const Handle<Object>& object) {
  // TODO(T25692962): Track "descriptorness" through a bit on the class
  HandleScope scope(thread);
  Handle<Type> type(&scope, typeOf(*object));
  return !lookupSymbolInMro(thread, type, SymbolId::kDunderDelete)->isError();
}

Object* Runtime::newCode() {
  HandleScope scope;
  Handle<Code> result(&scope, heap()->createCode());
  result->setArgcount(0);
  result->setKwonlyargcount(0);
  result->setCell2arg(0);
  result->setNlocals(0);
  result->setStacksize(0);
  result->setFlags(0);
  result->setFreevars(empty_object_array_);
  result->setCellvars(empty_object_array_);
  result->setFirstlineno(0);
  return *result;
}

Object* Runtime::newCoro() { return heap()->createCoro(); }

Object* Runtime::newBuiltinFunction(SymbolId name, Function::Entry entry,
                                    Function::Entry entry_kw,
                                    Function::Entry entry_ex) {
  Object* result = heap()->createFunction();
  DCHECK(result != Error::object(), "failed to createFunction");
  auto function = Function::cast(result);
  function->setName(symbols()->at(name));
  function->setEntry(entry);
  function->setEntryKw(entry_kw);
  function->setEntryEx(entry_ex);
  return result;
}

Object* Runtime::newFunction() {
  Object* object = heap()->createFunction();
  DCHECK(object != nullptr, "failed to createFunction");
  auto function = Function::cast(object);
  function->setEntry(unimplementedTrampoline);
  function->setEntryKw(unimplementedTrampoline);
  function->setEntryEx(unimplementedTrampoline);
  return function;
}

Object* Runtime::newGen() { return heap()->createGen(); }

Object* Runtime::newInstance(const Handle<Layout>& layout) {
  word num_words = layout->instanceSize();
  Object* object = heap()->createInstance(layout->id(), num_words);
  HeapObject* instance = HeapObject::cast(object);
  // Set the overflow array
  instance->instanceVariableAtPut(layout->overflowOffset(),
                                  empty_object_array_);
  return instance;
}

void Runtime::classAddBuiltinFunction(const Handle<Type>& type, SymbolId name,
                                      Function::Entry entry) {
  classAddBuiltinFunctionKwEx(type, name, entry, unimplementedTrampoline,
                              unimplementedTrampoline);
}

void Runtime::classAddBuiltinFunctionKw(const Handle<Type>& type, SymbolId name,
                                        Function::Entry entry,
                                        Function::Entry entry_kw) {
  classAddBuiltinFunctionKwEx(type, name, entry, entry_kw,
                              unimplementedTrampoline);
}

void Runtime::classAddBuiltinFunctionKwEx(const Handle<Type>& type,
                                          SymbolId name, Function::Entry entry,
                                          Function::Entry entry_kw,
                                          Function::Entry entry_ex) {
  HandleScope scope;
  Handle<Function> function(
      &scope, newBuiltinFunction(name, entry, entry_kw, entry_ex));
  Handle<Object> key(&scope, symbols()->at(name));
  Handle<Object> value(&scope, *function);
  Handle<Dict> dict(&scope, type->dict());
  dictAtPutInValueCell(dict, key, value);
}

void Runtime::classAddExtensionFunction(const Handle<Type>& type, SymbolId name,
                                        void* c_function) {
  DCHECK(!type->extensionType()->isNone(), "Type must contain extension type");

  HandleScope scope;
  Handle<Function> function(&scope, newFunction());
  Handle<Object> key(&scope, symbols()->at(name));
  function->setName(*key);
  function->setCode(newIntFromCPointer(c_function));
  function->setEntry(extensionTrampoline);
  function->setEntryKw(extensionTrampolineKw);
  function->setEntryEx(extensionTrampolineEx);
  Handle<Object> value(&scope, *function);
  Handle<Dict> dict(&scope, type->dict());
  dictAtPutInValueCell(dict, key, value);
}

Object* Runtime::newList() {
  HandleScope scope;
  Handle<List> result(&scope, heap()->createList());
  result->setAllocated(0);
  result->setItems(empty_object_array_);
  return *result;
}

Object* Runtime::newListIterator(const Handle<Object>& list) {
  HandleScope scope;
  Handle<ListIterator> list_iterator(&scope, heap()->createListIterator());
  list_iterator->setIndex(0);
  list_iterator->setList(*list);
  return *list_iterator;
}

Object* Runtime::newModule(const Handle<Object>& name) {
  HandleScope scope;
  Handle<Module> result(&scope, heap()->createModule());
  Handle<Dict> dict(&scope, newDict());
  result->setDict(*dict);
  result->setName(*name);
  result->setDef(newIntFromCPointer(nullptr));
  Handle<Object> key(&scope, symbols()->DunderName());
  dictAtPutInValueCell(dict, key, name);
  return *result;
}

Object* Runtime::newIntFromCPointer(void* ptr) {
  return newInt(reinterpret_cast<word>(ptr));
}

Object* Runtime::newObjectArray(word length) {
  if (length == 0) {
    return empty_object_array_;
  }
  return heap()->createObjectArray(length, None::object());
}

Object* Runtime::newInt(word value) {
  if (SmallInt::isValid(value)) {
    return SmallInt::fromWord(value);
  }
  return newIntWithDigits(View<uword>(reinterpret_cast<uword*>(&value), 1));
}

Object* Runtime::newFloat(double value) {
  return Float::cast(heap()->createFloat(value));
}

Object* Runtime::newComplex(double real, double imag) {
  return Complex::cast(heap()->createComplex(real, imag));
}

Object* Runtime::newIntWithDigits(View<uword> digits) {
  if (digits.length() == 0) {
    return SmallInt::fromWord(0);
  }
  if (digits.length() == 1) {
    uword u_digit = digits.get(0);
    word digit = *reinterpret_cast<word*>(&u_digit);
    if (SmallInt::isValid(digit)) {
      return SmallInt::fromWord(digit);
    }
  }
  HandleScope scope;
  Handle<LargeInt> result(&scope, heap()->createLargeInt(digits.length()));
  for (word i = 0; i < digits.length(); i++) {
    result->digitAtPut(i, digits.get(i));
  }
  return *result;
}

Object* Runtime::newProperty(const Handle<Object>& getter,
                             const Handle<Object>& setter,
                             const Handle<Object>& deleter) {
  HandleScope scope;
  Handle<Property> new_prop(&scope, heap()->createProperty());
  new_prop->setGetter(*getter);
  new_prop->setSetter(*setter);
  new_prop->setDeleter(*deleter);
  return *new_prop;
}

Object* Runtime::newRange(word start, word stop, word step) {
  auto range = Range::cast(heap()->createRange());
  range->setStart(start);
  range->setStop(stop);
  range->setStep(step);
  return range;
}

Object* Runtime::newRangeIterator(const Handle<Object>& range) {
  HandleScope scope;
  Handle<RangeIterator> range_iterator(&scope, heap()->createRangeIterator());
  range_iterator->setRange(*range);
  return *range_iterator;
}

Object* Runtime::newSetIterator(const Handle<Object>& set) {
  HandleScope scope;
  Handle<SetIterator> set_iterator(&scope, heap()->createSetIterator());
  set_iterator->setSet(*set);
  return *set_iterator;
}

Object* Runtime::newSlice(const Handle<Object>& start,
                          const Handle<Object>& stop,
                          const Handle<Object>& step) {
  HandleScope scope;
  Handle<Slice> slice(&scope, heap()->createSlice());
  slice->setStart(*start);
  slice->setStop(*stop);
  slice->setStep(*step);
  return *slice;
}

Object* Runtime::newStaticMethod() { return heap()->createStaticMethod(); }

Object* Runtime::newStrFromCStr(const char* c_str) {
  word length = std::strlen(c_str);
  auto data = reinterpret_cast<const byte*>(c_str);
  return newStrWithAll(View<byte>(data, length));
}

Object* Runtime::newStrFromFormat(const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  int length = std::vsnprintf(nullptr, 0, fmt, args);
  DCHECK(length >= 0, "Error occurred doing snprintf");
  va_end(args);
  va_start(args, fmt);
  char* buf = new char[length + 1];
  std::vsprintf(buf, fmt, args);
  va_end(args);
  Object* result = newStrFromCStr(buf);
  delete[] buf;
  return result;
}

Object* Runtime::newStrWithAll(View<byte> code_units) {
  word length = code_units.length();
  if (length <= SmallStr::kMaxLength) {
    return SmallStr::fromBytes(code_units);
  }
  Object* result = heap()->createLargeStr(length);
  DCHECK(result != Error::object(), "failed to create large string");
  byte* dst = reinterpret_cast<byte*>(LargeStr::cast(result)->address());
  const byte* src = code_units.data();
  memcpy(dst, src, length);
  return result;
}

Object* Runtime::internStrFromCStr(const char* c_str) {
  HandleScope scope;
  // TODO(T29648342): Optimize lookup to avoid creating an intermediary Str
  Handle<Object> str(&scope, newStrFromCStr(c_str));
  return internStr(str);
}

Object* Runtime::internStr(const Handle<Object>& str) {
  HandleScope scope;
  Handle<Set> set(&scope, interned());
  DCHECK(str->isStr(), "not a string");
  if (str->isSmallStr()) {
    return *str;
  }
  Handle<Object> key_hash(&scope, hash(*str));
  return setAddWithHash(set, str, key_hash);
}

Object* Runtime::hash(Object* object) {
  if (!object->isHeapObject()) {
    return immediateHash(object);
  }
  if (object->isByteArray() || object->isLargeStr()) {
    return valueHash(object);
  }
  return identityHash(object);
}

Object* Runtime::immediateHash(Object* object) {
  if (object->isSmallInt()) {
    return object;
  }
  if (object->isBool()) {
    return SmallInt::fromWord(Bool::cast(object)->value() ? 1 : 0);
  }
  if (object->isSmallStr()) {
    return SmallInt::fromWord(reinterpret_cast<uword>(object) >>
                              SmallStr::kTagSize);
  }
  return SmallInt::fromWord(reinterpret_cast<uword>(object));
}

// Xoroshiro128+
// http://xoroshiro.di.unimi.it/
uword Runtime::random() {
  const uint64_t s0 = random_state_[0];
  uint64_t s1 = random_state_[1];
  const uint64_t result = s0 + s1;
  s1 ^= s0;
  random_state_[0] = Utils::rotateLeft(s0, 55) ^ s1 ^ (s1 << 14);
  random_state_[1] = Utils::rotateLeft(s1, 36);
  return result;
}

void Runtime::setArgv(int argc, const char** argv) {
  HandleScope scope;
  Handle<List> list(&scope, newList());
  CHECK(argc >= 1, "Unexpected argc");
  for (int i = 1; i < argc; i++) {  // skip program name (i.e. "python")
    Handle<Object> arg_val(&scope, newStrFromCStr(argv[i]));
    listAdd(list, arg_val);
  }

  Handle<Object> module_name(&scope, symbols()->Sys());
  Handle<Module> sys_module(&scope, findModule(module_name));
  Handle<Object> argv_value(&scope, *list);
  moduleAddGlobal(sys_module, SymbolId::kArgv, argv_value);
}

Object* Runtime::identityHash(Object* object) {
  HeapObject* src = HeapObject::cast(object);
  word code = src->header()->hashCode();
  if (code == 0) {
    code = random() & Header::kHashCodeMask;
    code = (code == 0) ? 1 : code;
    src->setHeader(src->header()->withHashCode(code));
  }
  return SmallInt::fromWord(code);
}

word Runtime::siphash24(View<byte> array) {
  word result = 0;
  ::halfsiphash(array.data(), array.length(),
                reinterpret_cast<const uint8_t*>(hash_secret_),
                reinterpret_cast<uint8_t*>(&result), sizeof(result));
  return result;
}

Object* Runtime::valueHash(Object* object) {
  HeapObject* src = HeapObject::cast(object);
  Header* header = src->header();
  word code = header->hashCode();
  if (code == 0) {
    word size = src->headerCountOrOverflow();
    code = siphash24(View<byte>(reinterpret_cast<byte*>(src->address()), size));
    code &= Header::kHashCodeMask;
    code = (code == 0) ? 1 : code;
    src->setHeader(header->withHashCode(code));
    DCHECK(code == src->header()->hashCode(), "hash failure");
  }
  return SmallInt::fromWord(code);
}

void Runtime::initializeClasses() {
  initializeLayouts();
  initializeHeapClasses();
  initializeImmediateClasses();
}

void Runtime::initializeLayouts() {
  HandleScope scope;
  Handle<ObjectArray> array(&scope, newObjectArray(256));
  Handle<List> list(&scope, newList());
  list->setItems(*array);
  const word allocated = static_cast<word>(LayoutId::kLastBuiltinId) + 1;
  CHECK(allocated < array->length(), "bad allocation %ld", allocated);
  list->setAllocated(allocated);
  layouts_ = *list;
}

Object* Runtime::createMro(const Handle<Layout>& subclass_layout,
                           LayoutId superclass_id) {
  HandleScope scope;
  CHECK(subclass_layout->describedClass()->isType(),
        "subclass layout must have a described class");
  Handle<Type> superclass(&scope, typeAt(superclass_id));
  Handle<ObjectArray> src(&scope, superclass->mro());
  Handle<ObjectArray> dst(&scope, newObjectArray(1 + src->length()));
  dst->atPut(0, subclass_layout->describedClass());
  for (word i = 0; i < src->length(); i++) {
    dst->atPut(1 + i, src->at(i));
  }
  return *dst;
}

void Runtime::initializeHeapClasses() {
  ObjectBuiltins::initialize(this);

  // Abstract classes.
  StrBuiltins::initialize(this);
  IntBuiltins::initialize(this);

  // Exception hierarchy
  initializeExceptionClasses();

  // Concrete classes.

  addEmptyBuiltinClass(SymbolId::kByteArray, LayoutId::kByteArray,
                       LayoutId::kObject);
  initializeClassMethodClass();
  addEmptyBuiltinClass(SymbolId::kCode, LayoutId::kCode, LayoutId::kObject);
  ComplexBuiltins::initialize(this);
  DictBuiltins::initialize(this);
  addEmptyBuiltinClass(SymbolId::kEllipsis, LayoutId::kEllipsis,
                       LayoutId::kObject);
  FloatBuiltins::initialize(this);
  initializeFunctionClass();
  addEmptyBuiltinClass(SymbolId::kLargeStr, LayoutId::kLargeStr,
                       LayoutId::kStr);
  addEmptyBuiltinClass(SymbolId::kLayout, LayoutId::kLayout, LayoutId::kObject);
  ListBuiltins::initialize(this);
  ListIteratorBuiltins::initialize(this);
  addEmptyBuiltinClass(SymbolId::kMethod, LayoutId::kBoundMethod,
                       LayoutId::kObject);
  addEmptyBuiltinClass(SymbolId::kModule, LayoutId::kModule, LayoutId::kObject);
  addEmptyBuiltinClass(SymbolId::kNotImplementedType, LayoutId::kNotImplemented,
                       LayoutId::kObject);
  TupleBuiltins::initialize(this);
  TupleIteratorBuiltins::initialize(this);
  initializePropertyClass();
  RangeBuiltins::initialize(this);
  RangeIteratorBuiltins::initialize(this);
  initializeRefClass();
  SetBuiltins::initialize(this);
  SetIteratorBuiltins::initialize(this);
  addEmptyBuiltinClass(SymbolId::kSlice, LayoutId::kSlice, LayoutId::kObject);
  initializeStaticMethodClass();
  initializeSuperClass();
  initializeTypeClass();
  addEmptyBuiltinClass(SymbolId::kValueCell, LayoutId::kValueCell,
                       LayoutId::kObject);
}

void Runtime::initializeExceptionClasses() {
  BaseExceptionBuiltins::initialize(this);
  SystemExitBuiltins::initialize(this);
  addEmptyBuiltinClass(SymbolId::kException, LayoutId::kException,
                       LayoutId::kBaseException);
  addEmptyBuiltinClass(SymbolId::kAttributeError, LayoutId::kAttributeError,
                       LayoutId::kException);
  addEmptyBuiltinClass(SymbolId::kNameError, LayoutId::kNameError,
                       LayoutId::kException);
  addEmptyBuiltinClass(SymbolId::kTypeError, LayoutId::kTypeError,
                       LayoutId::kException);
  addEmptyBuiltinClass(SymbolId::kValueError, LayoutId::kValueError,
                       LayoutId::kException);

  // LookupError and subclasses.
  addEmptyBuiltinClass(SymbolId::kLookupError, LayoutId::kLookupError,
                       LayoutId::kException);
  addEmptyBuiltinClass(SymbolId::kIndexError, LayoutId::kIndexError,
                       LayoutId::kLookupError);
  addEmptyBuiltinClass(SymbolId::kKeyError, LayoutId::kKeyError,
                       LayoutId::kLookupError);

  // RuntimeError and subclasses.
  addEmptyBuiltinClass(SymbolId::kRuntimeError, LayoutId::kRuntimeError,
                       LayoutId::kException);
  addEmptyBuiltinClass(SymbolId::kNotImplementedError,
                       LayoutId::kNotImplementedError, LayoutId::kRuntimeError);

  StopIterationBuiltins::initialize(this);
  ImportErrorBuiltins::initialize(this);
  addEmptyBuiltinClass(SymbolId::kModuleNotFoundError,
                       LayoutId::kModuleNotFoundError, LayoutId::kImportError);
}

void Runtime::initializeRefClass() {
  HandleScope scope;
  Handle<Type> ref(&scope,
                   addEmptyBuiltinClass(SymbolId::kRef, LayoutId::kWeakRef,
                                        LayoutId::kObject));

  classAddBuiltinFunction(ref, SymbolId::kDunderInit,
                          nativeTrampoline<builtinRefInit>);

  classAddBuiltinFunction(ref, SymbolId::kDunderNew,
                          nativeTrampoline<builtinRefNew>);
}

void Runtime::initializeFunctionClass() {
  HandleScope scope;
  Handle<Type> function(
      &scope, addEmptyBuiltinClass(SymbolId::kFunction, LayoutId::kFunction,
                                   LayoutId::kObject));

  classAddBuiltinFunction(function, SymbolId::kDunderGet,
                          nativeTrampoline<builtinFunctionGet>);
}

void Runtime::initializeClassMethodClass() {
  HandleScope scope;
  Handle<Type> classmethod(
      &scope, addEmptyBuiltinClass(SymbolId::kClassmethod,
                                   LayoutId::kClassMethod, LayoutId::kObject));

  classAddBuiltinFunction(classmethod, SymbolId::kDunderGet,
                          nativeTrampoline<builtinClassMethodGet>);

  classAddBuiltinFunction(classmethod, SymbolId::kDunderInit,
                          nativeTrampoline<builtinClassMethodInit>);

  classAddBuiltinFunction(classmethod, SymbolId::kDunderNew,
                          nativeTrampoline<builtinClassMethodNew>);
}

void Runtime::initializeTypeClass() {
  HandleScope scope;
  Handle<Type> type(&scope,
                    addEmptyBuiltinClass(SymbolId::kType, LayoutId::kType,
                                         LayoutId::kObject));
  type->setFlag(Type::Flag::kTypeSubclass);

  classAddBuiltinFunctionKw(type, SymbolId::kDunderCall,
                            nativeTrampoline<builtinTypeCall>,
                            nativeTrampolineKw<builtinTypeCallKw>);

  classAddBuiltinFunction(type, SymbolId::kDunderInit,
                          nativeTrampoline<builtinTypeInit>);

  classAddBuiltinFunction(type, SymbolId::kDunderNew,
                          nativeTrampoline<builtinTypeNew>);
}

void Runtime::initializeImmediateClasses() {
  BoolBuiltins::initialize(this);
  NoneBuiltins::initialize(this);
  addEmptyBuiltinClass(SymbolId::kSmallStr, LayoutId::kSmallStr,
                       LayoutId::kStr);
  SmallIntBuiltins::initialize(this);
}

void Runtime::initializePropertyClass() {
  HandleScope scope;
  Handle<Type> property(
      &scope, addEmptyBuiltinClass(SymbolId::kProperty, LayoutId::kProperty,
                                   LayoutId::kObject));

  classAddBuiltinFunction(property, SymbolId::kDeleter,
                          nativeTrampoline<builtinPropertyDeleter>);

  classAddBuiltinFunction(property, SymbolId::kDunderGet,
                          nativeTrampoline<builtinPropertyDunderGet>);

  classAddBuiltinFunction(property, SymbolId::kDunderSet,
                          nativeTrampoline<builtinPropertyDunderSet>);

  classAddBuiltinFunction(property, SymbolId::kDunderInit,
                          nativeTrampoline<builtinPropertyInit>);

  classAddBuiltinFunction(property, SymbolId::kDunderNew,
                          nativeTrampoline<builtinPropertyNew>);

  classAddBuiltinFunction(property, SymbolId::kGetter,
                          nativeTrampoline<builtinPropertyGetter>);

  classAddBuiltinFunction(property, SymbolId::kSetter,
                          nativeTrampoline<builtinPropertySetter>);
}

void Runtime::initializeStaticMethodClass() {
  HandleScope scope;
  Handle<Type> staticmethod(
      &scope, addEmptyBuiltinClass(SymbolId::kStaticMethod,
                                   LayoutId::kStaticMethod, LayoutId::kObject));

  classAddBuiltinFunction(staticmethod, SymbolId::kDunderGet,
                          nativeTrampoline<builtinStaticMethodGet>);

  classAddBuiltinFunction(staticmethod, SymbolId::kDunderInit,
                          nativeTrampoline<builtinStaticMethodInit>);

  classAddBuiltinFunction(staticmethod, SymbolId::kDunderNew,
                          nativeTrampoline<builtinStaticMethodNew>);
}

void Runtime::collectGarbage() {
  bool run_callback = callbacks_ == None::object();
  Object* cb = Scavenger(this).scavenge();
  callbacks_ = WeakRef::spliceQueue(callbacks_, cb);
  if (run_callback) {
    processCallbacks();
  }
}

void Runtime::processCallbacks() {
  Thread* thread = Thread::currentThread();
  Frame* frame = thread->currentFrame();
  HandleScope scope(thread);
  while (callbacks_ != None::object()) {
    Handle<Object> weak(&scope, WeakRef::dequeueReference(&callbacks_));
    Handle<Object> callback(&scope, WeakRef::cast(*weak)->callback());
    Interpreter::callMethod1(thread, frame, callback, weak);
    thread->ignorePendingException();
    WeakRef::cast(*weak)->setCallback(None::object());
  }
}

Object* Runtime::run(const char* buffer) {
  HandleScope scope;

  Handle<Module> main(&scope, createMainModule());
  return executeModule(buffer, main);
}

Object* Runtime::runFromCStr(const char* c_str) {
  const char* buffer = compile(c_str);
  Object* result = run(buffer);
  delete[] buffer;
  return result;
}

Object* Runtime::executeModule(const char* buffer,
                               const Handle<Module>& module) {
  HandleScope scope;
  Marshal::Reader reader(&scope, this, buffer);

  reader.readLong();
  reader.readLong();
  reader.readLong();

  Handle<Code> code(&scope, reader.readObject());
  DCHECK(code->argcount() == 0, "invalid argcount %ld", code->argcount());

  return Thread::currentThread()->runModuleFunction(*module, *code);
}

extern "C" struct _inittab _PyImport_Inittab[];

Object* Runtime::importModule(const Handle<Object>& name) {
  HandleScope scope;
  Handle<Object> cached_module(&scope, findModule(name));
  if (!cached_module->isNone()) {
    return *cached_module;
  } else {
    for (int i = 0; _PyImport_Inittab[i].name != nullptr; i++) {
      if (Str::cast(*name)->equalsCStr(_PyImport_Inittab[i].name)) {
        (*_PyImport_Inittab[i].initfunc)();
        cached_module = findModule(name);
        return *cached_module;
      }
    }
  }

  return Thread::currentThread()->throwRuntimeErrorFromCStr(
      "importModule is unimplemented!");
}

// TODO: support fromlist and level. Ideally, we'll never implement that
// functionality in c++, instead using the pure-python importlib
// implementation that ships with cpython.
Object* Runtime::importModuleFromBuffer(const char* buffer,
                                        const Handle<Object>& name) {
  HandleScope scope;
  Handle<Object> cached_module(&scope, findModule(name));
  if (!cached_module->isNone()) {
    return *cached_module;
  }

  Handle<Module> module(&scope, newModule(name));
  addModule(module);
  executeModule(buffer, module);
  return *module;
}

void Runtime::initializeThreads() {
  auto main_thread = new Thread(Thread::kDefaultStackSize);
  threads_ = main_thread;
  main_thread->setRuntime(this);
  Thread::setCurrentThread(main_thread);
}

void Runtime::initializePrimitiveInstances() {
  empty_object_array_ = heap()->createObjectArray(0, None::object());
  empty_byte_array_ = heap()->createByteArray(0);
  ellipsis_ = heap()->createEllipsis();
  not_implemented_ = heap()->createNotImplemented();
  callbacks_ = None::object();
}

void Runtime::initializeInterned() { interned_ = newSet(); }

void Runtime::initializeRandom() {
  uword random_state[2];
  uword hash_secret[2];
  OS::secureRandom(reinterpret_cast<byte*>(&random_state),
                   sizeof(random_state));
  OS::secureRandom(reinterpret_cast<byte*>(&hash_secret), sizeof(hash_secret));
  seedRandom(random_state, hash_secret);
}

void Runtime::initializeSymbols() {
  HandleScope scope;
  symbols_ = new Symbols(this);
  for (int i = 0; i < static_cast<int>(SymbolId::kMaxId); i++) {
    SymbolId id = static_cast<SymbolId>(i);
    Handle<Object> symbol(&scope, symbols()->at(id));
    internStr(symbol);
  }
}

void Runtime::visitRoots(PointerVisitor* visitor) {
  visitRuntimeRoots(visitor);
  visitThreadRoots(visitor);
}

void Runtime::visitRuntimeRoots(PointerVisitor* visitor) {
  // Visit layouts
  visitor->visitPointer(&layouts_);

  // Visit instances
  visitor->visitPointer(&empty_byte_array_);
  visitor->visitPointer(&empty_object_array_);
  visitor->visitPointer(&ellipsis_);
  visitor->visitPointer(&not_implemented_);
  visitor->visitPointer(&build_class_);
  visitor->visitPointer(&display_hook_);

  // Visit interned strings.
  visitor->visitPointer(&interned_);

  // Visit modules
  visitor->visitPointer(&modules_);

  // Visit C-API handles
  visitor->visitPointer(&api_handles_);

  // Visit symbols
  symbols_->visit(visitor);

  // Visit GC callbacks
  visitor->visitPointer(&callbacks_);
}

void Runtime::visitThreadRoots(PointerVisitor* visitor) {
  for (Thread* thread = threads_; thread != nullptr; thread = thread->next()) {
    thread->visitRoots(visitor);
  }
}

void Runtime::addModule(const Handle<Module>& module) {
  HandleScope scope;
  Handle<Dict> dict(&scope, modules());
  Handle<Object> key(&scope, module->name());
  Handle<Object> value(&scope, *module);
  dictAtPut(dict, key, value);
}

Object* Runtime::findModule(const Handle<Object>& name) {
  DCHECK(name->isStr(), "name not a string");

  HandleScope scope;
  Handle<Dict> dict(&scope, modules());
  Object* value = dictAt(dict, name);
  if (value->isError()) {
    return None::object();
  }
  return value;
}

Object* Runtime::moduleAt(const Handle<Module>& module,
                          const Handle<Object>& key) {
  HandleScope scope;
  Handle<Dict> dict(&scope, module->dict());
  Handle<Object> value_cell(&scope, dictAt(dict, key));
  if (value_cell->isError()) {
    return Error::object();
  }
  return ValueCell::cast(*value_cell)->value();
}

void Runtime::moduleAtPut(const Handle<Module>& module,
                          const Handle<Object>& key,
                          const Handle<Object>& value) {
  HandleScope scope;
  Handle<Dict> dict(&scope, module->dict());
  dictAtPutInValueCell(dict, key, value);
}

struct {
  SymbolId name;
  void (Runtime::*create_module)();
} kBuiltinModules[] = {
    {SymbolId::kBuiltins, &Runtime::createBuiltinsModule},
    {SymbolId::kSys, &Runtime::createSysModule},
    {SymbolId::kTime, &Runtime::createTimeModule},
    {SymbolId::kUnderWeakRef, &Runtime::createWeakRefModule},
};

void Runtime::initializeModules() {
  modules_ = newDict();
  for (uword i = 0; i < ARRAYSIZE(kBuiltinModules); i++) {
    (this->*kBuiltinModules[i].create_module)();
  }
}

void Runtime::initializeApiHandles() { api_handles_ = newDict(); }

Object* Runtime::typeOf(Object* object) {
  HandleScope scope;
  Handle<Layout> layout(&scope, layoutAt(object->layoutId()));
  return layout->describedClass();
}

Object* Runtime::layoutAt(LayoutId layout_id) {
  return List::cast(layouts_)->at(static_cast<word>(layout_id));
}

void Runtime::layoutAtPut(LayoutId layout_id, Object* object) {
  List::cast(layouts_)->atPut(static_cast<word>(layout_id), object);
}

Object* Runtime::typeAt(LayoutId layout_id) {
  return Layout::cast(layoutAt(layout_id))->describedClass();
}

LayoutId Runtime::reserveLayoutId() {
  HandleScope scope;
  Handle<List> list(&scope, layouts_);
  Handle<Object> value(&scope, None::object());
  word result = list->allocated();
  DCHECK(result <= Header::kMaxLayoutId,
         "exceeded layout id space in header word");
  listAdd(list, value);
  return static_cast<LayoutId>(result);
}

SymbolId Runtime::binaryOperationSelector(Interpreter::BinaryOp op) {
  return kBinaryOperationSelector[static_cast<int>(op)];
}

SymbolId Runtime::swappedBinaryOperationSelector(Interpreter::BinaryOp op) {
  return kSwappedBinaryOperationSelector[static_cast<int>(op)];
}

SymbolId Runtime::inplaceOperationSelector(Interpreter::BinaryOp op) {
  DCHECK(op != Interpreter::BinaryOp::DIVMOD,
         "DIVMOD is not a valid inplace op");
  return kInplaceOperationSelector[static_cast<int>(op)];
}

SymbolId Runtime::comparisonSelector(CompareOp op) {
  DCHECK(op >= CompareOp::LT, "invalid compare op");
  DCHECK(op <= CompareOp::GE, "invalid compare op");
  return kComparisonSelector[op];
}

SymbolId Runtime::swappedComparisonSelector(CompareOp op) {
  DCHECK(op >= CompareOp::LT, "invalid compare op");
  DCHECK(op <= CompareOp::GE, "invalid compare op");
  CompareOp swapped_op = kSwappedCompareOp[op];
  return comparisonSelector(swapped_op);
}

bool Runtime::isMethodOverloaded(Thread* thread, const Handle<Type>& type,
                                 SymbolId selector) {
  HandleScope scope(thread);
  Handle<ObjectArray> mro(&scope, type->mro());
  Handle<Object> key(&scope, symbols()->at(selector));
  DCHECK(mro->length() > 0, "empty MRO");
  for (word i = 0; i < mro->length() - 1; i++) {
    Handle<Type> mro_type(&scope, mro->at(i));
    Handle<Dict> dict(&scope, mro_type->dict());
    Handle<Object> value_cell(&scope, dictAt(dict, key));
    if (!value_cell->isError()) {
      return true;
    }
  }
  return false;
}

Object* Runtime::moduleAddGlobal(const Handle<Module>& module, SymbolId name,
                                 const Handle<Object>& value) {
  HandleScope scope;
  Handle<Dict> dict(&scope, module->dict());
  Handle<Object> key(&scope, symbols()->at(name));
  return dictAtPutInValueCell(dict, key, value);
}

Object* Runtime::moduleAddBuiltinFunction(const Handle<Module>& module,
                                          SymbolId name,
                                          const Function::Entry entry,
                                          const Function::Entry entry_kw,
                                          const Function::Entry entry_ex) {
  HandleScope scope;
  Handle<Object> key(&scope, symbols()->at(name));
  Handle<Dict> dict(&scope, module->dict());
  Handle<Object> value(&scope,
                       newBuiltinFunction(name, entry, entry_kw, entry_ex));
  return dictAtPutInValueCell(dict, key, value);
}

void Runtime::createBuiltinsModule() {
  HandleScope scope;
  Handle<Object> name(&scope, newStrFromCStr("builtins"));
  Handle<Module> module(&scope, newModule(name));

  // Fill in builtins...
  build_class_ = moduleAddBuiltinFunction(
      module, SymbolId::kDunderBuildClass, nativeTrampoline<builtinBuildClass>,
      nativeTrampolineKw<builtinBuildClassKw>, unimplementedTrampoline);

  moduleAddBuiltinFunction(module, SymbolId::kCallable,
                           nativeTrampoline<builtinCallable>,
                           unimplementedTrampoline, unimplementedTrampoline);
  moduleAddBuiltinFunction(module, SymbolId::kChr, nativeTrampoline<builtinChr>,
                           unimplementedTrampoline, unimplementedTrampoline);
  moduleAddBuiltinFunction(module, SymbolId::kGetattr,
                           nativeTrampoline<builtinGetattr>,
                           unimplementedTrampoline, unimplementedTrampoline);
  moduleAddBuiltinFunction(module, SymbolId::kHasattr,
                           nativeTrampoline<builtinHasattr>,
                           unimplementedTrampoline, unimplementedTrampoline);
  moduleAddBuiltinFunction(module, SymbolId::kIsInstance,
                           nativeTrampoline<builtinIsinstance>,
                           unimplementedTrampoline, unimplementedTrampoline);
  moduleAddBuiltinFunction(module, SymbolId::kIsSubclass,
                           nativeTrampoline<builtinIssubclass>,
                           unimplementedTrampoline, unimplementedTrampoline);
  moduleAddBuiltinFunction(module, SymbolId::kLen, nativeTrampoline<builtinLen>,
                           unimplementedTrampoline, unimplementedTrampoline);
  moduleAddBuiltinFunction(module, SymbolId::kOrd, nativeTrampoline<builtinOrd>,
                           unimplementedTrampoline, unimplementedTrampoline);
  moduleAddBuiltinFunction(
      module, SymbolId::kPrint, nativeTrampoline<builtinPrint>,
      nativeTrampolineKw<builtinPrintKw>, unimplementedTrampoline);
  moduleAddBuiltinFunction(module, SymbolId::kRange,
                           nativeTrampoline<builtinRange>,
                           unimplementedTrampoline, unimplementedTrampoline);
  moduleAddBuiltinFunction(module, SymbolId::kRepr,
                           nativeTrampoline<builtinRepr>,
                           unimplementedTrampoline, unimplementedTrampoline);
  moduleAddBuiltinFunction(module, SymbolId::kSetattr,
                           nativeTrampoline<builtinSetattr>,
                           unimplementedTrampoline, unimplementedTrampoline);
  // Add builtin types
  moduleAddBuiltinType(module, SymbolId::kAttributeError,
                       LayoutId::kAttributeError);
  moduleAddBuiltinType(module, SymbolId::kBaseException,
                       LayoutId::kBaseException);
  moduleAddBuiltinType(module, SymbolId::kBool, LayoutId::kBool);
  moduleAddBuiltinType(module, SymbolId::kClassmethod, LayoutId::kClassMethod);
  moduleAddBuiltinType(module, SymbolId::kComplex, LayoutId::kComplex);
  moduleAddBuiltinType(module, SymbolId::kDict, LayoutId::kDict);
  moduleAddBuiltinType(module, SymbolId::kException, LayoutId::kException);
  moduleAddBuiltinType(module, SymbolId::kFloat, LayoutId::kFloat);
  moduleAddBuiltinType(module, SymbolId::kImportError, LayoutId::kImportError);
  moduleAddBuiltinType(module, SymbolId::kInt, LayoutId::kInt);
  moduleAddBuiltinType(module, SymbolId::kIndexError, LayoutId::kIndexError);
  moduleAddBuiltinType(module, SymbolId::kKeyError, LayoutId::kKeyError);
  moduleAddBuiltinType(module, SymbolId::kList, LayoutId::kList);
  moduleAddBuiltinType(module, SymbolId::kLookupError, LayoutId::kLookupError);
  moduleAddBuiltinType(module, SymbolId::kModuleNotFoundError,
                       LayoutId::kModuleNotFoundError);
  moduleAddBuiltinType(module, SymbolId::kNameError, LayoutId::kNameError);
  moduleAddBuiltinType(module, SymbolId::kNotImplementedError,
                       LayoutId::kNotImplementedError);
  moduleAddBuiltinType(module, SymbolId::kObjectClassname, LayoutId::kObject);
  moduleAddBuiltinType(module, SymbolId::kProperty, LayoutId::kProperty);
  moduleAddBuiltinType(module, SymbolId::kRuntimeError,
                       LayoutId::kRuntimeError);
  moduleAddBuiltinType(module, SymbolId::kStaticMethod,
                       LayoutId::kStaticMethod);
  moduleAddBuiltinType(module, SymbolId::kSet, LayoutId::kSet);
  moduleAddBuiltinType(module, SymbolId::kStopIteration,
                       LayoutId::kStopIteration);
  moduleAddBuiltinType(module, SymbolId::kStr, LayoutId::kStr);
  moduleAddBuiltinType(module, SymbolId::kSystemExit, LayoutId::kSystemExit);
  moduleAddBuiltinType(module, SymbolId::kSuper, LayoutId::kSuper);
  moduleAddBuiltinType(module, SymbolId::kTuple, LayoutId::kObjectArray);
  moduleAddBuiltinType(module, SymbolId::kType, LayoutId::kType);
  moduleAddBuiltinType(module, SymbolId::kTypeError, LayoutId::kTypeError);
  moduleAddBuiltinType(module, SymbolId::kValueError, LayoutId::kValueError);

  Handle<Object> not_implemented(&scope, notImplemented());
  moduleAddGlobal(module, SymbolId::kNotImplemented, not_implemented);

  addModule(module);
  executeModule(kBuiltinsModuleData, module);
}

void Runtime::moduleAddBuiltinType(const Handle<Module>& module, SymbolId name,
                                   LayoutId layout_id) {
  HandleScope scope;
  Handle<Object> value(&scope, typeAt(layout_id));
  moduleAddGlobal(module, name, value);
}

void Runtime::moduleImportAllFrom(const Handle<Dict>& dict,
                                  const Handle<Module>& module) {
  HandleScope scope;
  Handle<Dict> module_dict(&scope, module->dict());
  Handle<ObjectArray> module_keys(&scope, dictKeys(module_dict));
  for (word i = 0; i < module_keys->length(); i++) {
    Handle<Object> symbol_name(&scope, module_keys->at(i));
    CHECK(symbol_name->isStr(), "Symbol is not a String");

    // Load all the symbols not starting with '_'
    Handle<Str> symbol_name_str(&scope, *symbol_name);
    if (symbol_name_str->charAt(0) != '_') {
      Handle<Object> value(&scope, moduleAt(module, symbol_name));
      dictAtPutInValueCell(dict, symbol_name, value);
    }
  }
}

void Runtime::createSysModule() {
  HandleScope scope;
  Handle<Object> name(&scope, symbols()->Sys());
  Handle<Module> module(&scope, newModule(name));

  Handle<Object> modules(&scope, modules_);
  moduleAddGlobal(module, SymbolId::kModules, modules);

  display_hook_ = moduleAddBuiltinFunction(
      module, SymbolId::kDisplayhook, nativeTrampoline<builtinSysDisplayhook>,
      unimplementedTrampoline, unimplementedTrampoline);

  // Fill in sys...
  moduleAddBuiltinFunction(module, SymbolId::kExit,
                           nativeTrampoline<builtinSysExit>,
                           unimplementedTrampoline, unimplementedTrampoline);

  Handle<Object> stdout_val(&scope, SmallInt::fromWord(STDOUT_FILENO));
  moduleAddGlobal(module, SymbolId::kStdout, stdout_val);

  Handle<Object> stderr_val(&scope, SmallInt::fromWord(STDERR_FILENO));
  moduleAddGlobal(module, SymbolId::kStderr, stderr_val);

  Handle<Object> meta_path(&scope, newList());
  moduleAddGlobal(module, SymbolId::kMetaPath, meta_path);

  Handle<Object> platform(&scope, newStrFromCStr(OS::name()));
  moduleAddGlobal(module, SymbolId::kPlatform, platform);

  // Count the number of modules and create a tuple
  word num_external_modules = 0;
  while (_PyImport_Inittab[num_external_modules].name != nullptr) {
    num_external_modules++;
  }
  word num_modules = ARRAYSIZE(kBuiltinModules) + num_external_modules;
  Handle<ObjectArray> builtins_tuple(&scope, newObjectArray(num_modules));

  // Add all the available builtin modules
  for (uword i = 0; i < ARRAYSIZE(kBuiltinModules); i++) {
    Handle<Object> module_name(&scope, symbols()->at(kBuiltinModules[i].name));
    builtins_tuple->atPut(i, *module_name);
  }

  // Add all the available extension builtin modules
  for (int i = 0; _PyImport_Inittab[i].name != nullptr; i++) {
    Handle<Object> module_name(&scope,
                               newStrFromCStr(_PyImport_Inittab[i].name));
    builtins_tuple->atPut(ARRAYSIZE(kBuiltinModules) + i, *module_name);
  }

  // Create builtin_module_names tuple
  Handle<Object> builtins(&scope, *builtins_tuple);
  moduleAddGlobal(module, SymbolId::kBuiltinModuleNames, builtins);

  addModule(module);
}

void Runtime::createWeakRefModule() {
  HandleScope scope;
  Handle<Object> name(&scope, symbols()->UnderWeakRef());
  Handle<Module> module(&scope, newModule(name));

  moduleAddBuiltinType(module, SymbolId::kRef, LayoutId::kWeakRef);
  addModule(module);
}

void Runtime::createTimeModule() {
  HandleScope scope;
  Handle<Object> name(&scope, symbols()->Time());
  Handle<Module> module(&scope, newModule(name));

  // time.time
  moduleAddBuiltinFunction(module, SymbolId::kTime,
                           nativeTrampoline<builtinTime>,
                           unimplementedTrampoline, unimplementedTrampoline);

  addModule(module);
}

Object* Runtime::createMainModule() {
  HandleScope scope;
  Handle<Object> name(&scope, symbols()->DunderMain());
  Handle<Module> module(&scope, newModule(name));

  // Fill in __main__...

  addModule(module);

  return *module;
}

// List

void Runtime::listEnsureCapacity(const Handle<List>& list, word index) {
  if (index < list->capacity()) {
    return;
  }
  HandleScope scope;
  word new_capacity = (list->capacity() < kInitialEnsuredCapacity)
                          ? kInitialEnsuredCapacity
                          : list->capacity() << 1;
  if (new_capacity < index) {
    new_capacity = Utils::nextPowerOfTwo(index);
  }
  Handle<ObjectArray> old_array(&scope, list->items());
  Handle<ObjectArray> new_array(&scope, newObjectArray(new_capacity));
  old_array->copyTo(*new_array);
  list->setItems(*new_array);
}

void Runtime::listAdd(const Handle<List>& list, const Handle<Object>& value) {
  HandleScope scope;
  word index = list->allocated();
  listEnsureCapacity(list, index);
  list->setAllocated(index + 1);
  list->atPut(index, *value);
}

Object* Runtime::listExtend(Thread* thread, const Handle<List>& dst,
                            const Handle<Object>& iterable) {
  HandleScope scope(thread);
  Handle<Object> elt(&scope, None::object());
  word index = dst->allocated();
  // Special case for lists
  if (iterable->isList()) {
    Handle<List> src(&scope, *iterable);
    if (src->allocated() > 0) {
      word new_capacity = index + src->allocated();
      listEnsureCapacity(dst, new_capacity);
      dst->setAllocated(new_capacity);
      for (word i = 0; i < src->allocated(); i++) {
        dst->atPut(index++, src->at(i));
      }
    }
    return *dst;
  }
  // Special case for list iterators
  if (iterable->isListIterator()) {
    Handle<ListIterator> list_iter(&scope, *iterable);
    Handle<List> src(&scope, list_iter->list());
    word new_capacity = index + src->allocated();
    listEnsureCapacity(dst, new_capacity);
    dst->setAllocated(new_capacity);
    for (word i = 0; i < src->allocated(); i++) {
      elt = list_iter->next();
      if (elt->isError()) {
        break;
      }
      dst->atPut(index++, src->at(i));
    }
    return *dst;
  }
  // Special case for tuples
  if (iterable->isObjectArray()) {
    Handle<ObjectArray> tuple(&scope, *iterable);
    if (tuple->length() > 0) {
      word new_capacity = index + tuple->length();
      listEnsureCapacity(dst, new_capacity);
      dst->setAllocated(new_capacity);
      for (word i = 0; i < tuple->length(); i++) {
        dst->atPut(index++, tuple->at(i));
      }
    }
    return *dst;
  }
  // Special case for sets
  if (iterable->isSet()) {
    Handle<Set> set(&scope, *iterable);
    if (set->numItems() > 0) {
      Handle<ObjectArray> data(&scope, set->data());
      word new_capacity = index + set->numItems();
      listEnsureCapacity(dst, new_capacity);
      dst->setAllocated(new_capacity);
      for (word i = 0; i < data->length(); i += Set::Bucket::kNumPointers) {
        if (Set::Bucket::isEmpty(*data, i) ||
            Set::Bucket::isTombstone(*data, i)) {
          continue;
        }
        dst->atPut(index++, Set::Bucket::key(*data, i));
      }
    }
    return *dst;
  }
  // Special case for dicts
  if (iterable->isDict()) {
    Handle<Dict> dict(&scope, *iterable);
    if (dict->numItems() > 0) {
      Handle<ObjectArray> keys(&scope, dictKeys(dict));
      word new_capacity = index + dict->numItems();
      listEnsureCapacity(dst, new_capacity);
      dst->setAllocated(new_capacity);
      for (word i = 0; i < keys->length(); i++) {
        dst->atPut(index++, keys->at(i));
      }
    }
    return *dst;
  }
  // Generic case
  Handle<Object> iter_method(
      &scope, Interpreter::lookupMethod(thread, thread->currentFrame(),
                                        iterable, SymbolId::kDunderIter));
  if (iter_method->isError()) {
    return thread->throwTypeErrorFromCStr("object is not iterable");
  }
  Handle<Object> iterator(
      &scope, Interpreter::callMethod1(thread, thread->currentFrame(),
                                       iter_method, iterable));
  if (iterator->isError()) {
    return thread->throwTypeErrorFromCStr("object is not iterable");
  }
  Handle<Object> next_method(
      &scope, Interpreter::lookupMethod(thread, thread->currentFrame(),
                                        iterator, SymbolId::kDunderNext));
  if (next_method->isError()) {
    return thread->throwTypeErrorFromCStr("iter() returned a non-iterator");
  }
  Handle<Object> value(&scope, None::object());
  while (!isIteratorExhausted(thread, iterator)) {
    value = Interpreter::callMethod1(thread, thread->currentFrame(),
                                     next_method, iterator);
    if (value->isError()) {
      return *value;
    }
    listAdd(dst, value);
  }
  return *dst;
}

void Runtime::listInsert(const Handle<List>& list, const Handle<Object>& value,
                         word index) {
  listAdd(list, value);
  word last_index = list->allocated() - 1;
  if (index < 0) {
    index = last_index + index;
  }
  index =
      Utils::maximum(static_cast<word>(0), Utils::minimum(last_index, index));
  for (word i = last_index; i > index; i--) {
    list->atPut(i, list->at(i - 1));
  }
  list->atPut(index, *value);
}

Object* Runtime::listPop(const Handle<List>& list, word index) {
  HandleScope scope;
  Handle<Object> popped(&scope, list->at(index));
  list->atPut(index, None::object());
  word last_index = list->allocated() - 1;
  for (word i = index; i < last_index; i++) {
    list->atPut(i, list->at(i + 1));
  }
  list->setAllocated(list->allocated() - 1);
  return *popped;
}

Object* Runtime::listReplicate(Thread* thread, const Handle<List>& list,
                               word ntimes) {
  HandleScope scope(thread);
  word len = list->allocated();
  Handle<ObjectArray> items(&scope, newObjectArray(ntimes * len));
  for (word i = 0; i < ntimes; i++) {
    for (word j = 0; j < len; j++) {
      items->atPut((i * len) + j, list->at(j));
    }
  }
  Handle<List> result(&scope, newList());
  result->setItems(*items);
  result->setAllocated(items->length());
  return *result;
}

char* Runtime::compile(const char* src) {
  // increment this if you change the caching code, to invalidate existing
  // cache entries.
  uint64_t seed[2] = {0, 1};
  word hash = 0;

  // Hash the input.
  ::siphash(reinterpret_cast<const uint8_t*>(src), strlen(src),
            reinterpret_cast<const uint8_t*>(seed),
            reinterpret_cast<uint8_t*>(&hash), sizeof(hash));

  const char* cache_env = OS::getenv("PYRO_CACHE_DIR");
  std::string cache_dir;
  if (cache_env != nullptr) {
    cache_dir = cache_env;
  } else {
    const char* home_env = OS::getenv("HOME");
    if (home_env != nullptr) {
      cache_dir = home_env;
      cache_dir += "/.pyro-compile-cache";
    }
  }

  char filename_buf[512] = {};
  snprintf(filename_buf, 512, "%s/%016zx", cache_dir.c_str(), hash);

  // Read compiled code from the cache
  if (!cache_dir.empty() && OS::fileExists(filename_buf)) {
    return OS::readFile(filename_buf);
  }

  // Cache miss, must run the compiler.
  std::unique_ptr<char[]> tmp_dir(OS::temporaryDirectory("python-tests"));
  const std::string dir(tmp_dir.get());
  const std::string py = dir + "/foo.py";
  const std::string pyc = dir + "/foo.pyc";
  const std::string cleanup = "rm -rf " + dir;
  std::ofstream output(py);
  output << src;
  output.close();
  const std::string command =
      "/usr/local/fbcode/gcc-5-glibc-2.23/bin/python3.6 -m compileall -q -b " +
      py;
  system(command.c_str());
  word len;
  char* result = OS::readFile(pyc.c_str(), &len);
  system(cleanup.c_str());

  // Cache the output if possible.
  if (!cache_dir.empty() && OS::dirExists(cache_dir.c_str())) {
    OS::writeFileExcl(filename_buf, result, len);
  }

  return result;
}

// Dict

Object* Runtime::newDict() {
  HandleScope scope;
  Handle<Dict> result(&scope, heap()->createDict());
  result->setNumItems(0);
  result->setData(empty_object_array_);
  return *result;
}

Object* Runtime::newDict(word initial_size) {
  HandleScope scope;
  // TODO: initialSize should be scaled up by a load factor.
  word initial_capacity = Utils::nextPowerOfTwo(initial_size);
  Handle<ObjectArray> array(
      &scope,
      newObjectArray(Utils::maximum(static_cast<word>(kInitialDictCapacity),
                                    initial_capacity) *
                     Dict::Bucket::kNumPointers));
  Handle<Dict> result(&scope, newDict());
  result->setData(*array);
  return *result;
}

void Runtime::dictAtPutWithHash(const Handle<Dict>& dict,
                                const Handle<Object>& key,
                                const Handle<Object>& value,
                                const Handle<Object>& key_hash) {
  HandleScope scope;
  Handle<ObjectArray> data(&scope, dict->data());
  word index = -1;
  bool found = dictLookup(data, key, key_hash, &index);
  if (index == -1) {
    // TODO(mpage): Grow at a predetermined load factor, rather than when full
    Handle<ObjectArray> new_data(&scope, dictGrow(data));
    dictLookup(new_data, key, key_hash, &index);
    DCHECK(index != -1, "invalid index %ld", index);
    dict->setData(*new_data);
    Dict::Bucket::set(*new_data, index, *key_hash, *key, *value);
  } else {
    Dict::Bucket::set(*data, index, *key_hash, *key, *value);
  }
  if (!found) {
    dict->setNumItems(dict->numItems() + 1);
  }
}

void Runtime::dictAtPut(const Handle<Dict>& dict, const Handle<Object>& key,
                        const Handle<Object>& value) {
  HandleScope scope;
  Handle<Object> key_hash(&scope, hash(*key));
  return dictAtPutWithHash(dict, key, value, key_hash);
}

ObjectArray* Runtime::dictGrow(const Handle<ObjectArray>& data) {
  HandleScope scope;
  word new_length = data->length() * kDictGrowthFactor;
  if (new_length == 0) {
    new_length = kInitialDictCapacity * Dict::Bucket::kNumPointers;
  }
  Handle<ObjectArray> new_data(&scope, newObjectArray(new_length));
  // Re-insert items
  for (word i = 0; i < data->length(); i += Dict::Bucket::kNumPointers) {
    if (Dict::Bucket::isEmpty(*data, i) ||
        Dict::Bucket::isTombstone(*data, i)) {
      continue;
    }
    Handle<Object> key(&scope, Dict::Bucket::key(*data, i));
    Handle<Object> hash(&scope, Dict::Bucket::hash(*data, i));
    word index = -1;
    dictLookup(new_data, key, hash, &index);
    DCHECK(index != -1, "invalid index %ld", index);
    Dict::Bucket::set(*new_data, index, *hash, *key,
                      Dict::Bucket::value(*data, i));
  }
  return *new_data;
}

Object* Runtime::dictAtWithHash(const Handle<Dict>& dict,
                                const Handle<Object>& key,
                                const Handle<Object>& key_hash) {
  HandleScope scope;
  Handle<ObjectArray> data(&scope, dict->data());
  word index = -1;
  bool found = dictLookup(data, key, key_hash, &index);
  if (found) {
    DCHECK(index != -1, "invalid index %ld", index);
    return Dict::Bucket::value(*data, index);
  }
  return Error::object();
}

Object* Runtime::dictAt(const Handle<Dict>& dict, const Handle<Object>& key) {
  HandleScope scope;
  Handle<Object> key_hash(&scope, hash(*key));
  return dictAtWithHash(dict, key, key_hash);
}

Object* Runtime::dictAtIfAbsentPut(const Handle<Dict>& dict,
                                   const Handle<Object>& key,
                                   Callback<Object*>* thunk) {
  HandleScope scope;
  Handle<ObjectArray> data(&scope, dict->data());
  word index = -1;
  Handle<Object> key_hash(&scope, hash(*key));
  bool found = dictLookup(data, key, key_hash, &index);
  if (found) {
    DCHECK(index != -1, "invalid index %ld", index);
    return Dict::Bucket::value(*data, index);
  }
  Handle<Object> value(&scope, thunk->call());
  if (index == -1) {
    // TODO(mpage): Grow at a predetermined load factor, rather than when full
    Handle<ObjectArray> new_data(&scope, dictGrow(data));
    dictLookup(new_data, key, key_hash, &index);
    DCHECK(index != -1, "invalid index %ld", index);
    dict->setData(*new_data);
    Dict::Bucket::set(*new_data, index, *key_hash, *key, *value);
  } else {
    Dict::Bucket::set(*data, index, *key_hash, *key, *value);
  }
  dict->setNumItems(dict->numItems() + 1);
  return *value;
}

Object* Runtime::dictAtPutInValueCell(const Handle<Dict>& dict,
                                      const Handle<Object>& key,
                                      const Handle<Object>& value) {
  Object* result = dictAtIfAbsentPut(dict, key, newValueCellCallback());
  ValueCell::cast(result)->setValue(*value);
  return result;
}

bool Runtime::dictIncludes(const Handle<Dict>& dict,
                           const Handle<Object>& key) {
  HandleScope scope;
  Handle<ObjectArray> data(&scope, dict->data());
  Handle<Object> key_hash(&scope, hash(*key));
  word ignore;
  return dictLookup(data, key, key_hash, &ignore);
}

bool Runtime::dictRemove(const Handle<Dict>& dict, const Handle<Object>& key,
                         Object** value) {
  HandleScope scope;
  Handle<ObjectArray> data(&scope, dict->data());
  word index = -1;
  Handle<Object> key_hash(&scope, hash(*key));
  bool found = dictLookup(data, key, key_hash, &index);
  if (found) {
    DCHECK(index != -1, "unexpected index %ld", index);
    if (value != nullptr) {
      *value = Dict::Bucket::value(*data, index);
    }
    Dict::Bucket::setTombstone(*data, index);
    dict->setNumItems(dict->numItems() - 1);
  }
  return found;
}

bool Runtime::dictLookup(const Handle<ObjectArray>& data,
                         const Handle<Object>& key,
                         const Handle<Object>& key_hash, word* index) {
  word start = Dict::Bucket::getIndex(*data, *key_hash);
  word current = start;
  word next_free_index = -1;

  // TODO(mpage) - Quadratic probing?
  word length = data->length();
  if (length == 0) {
    *index = -1;
    return false;
  }

  do {
    if (Dict::Bucket::hasKey(*data, current, *key)) {
      *index = current;
      return true;
    } else if (next_free_index == -1 &&
               Dict::Bucket::isTombstone(*data, current)) {
      next_free_index = current;
    } else if (Dict::Bucket::isEmpty(*data, current)) {
      if (next_free_index == -1) {
        next_free_index = current;
      }
      break;
    }
    current = (current + Dict::Bucket::kNumPointers) % length;
  } while (current != start);

  *index = next_free_index;

  return false;
}

ObjectArray* Runtime::dictKeys(const Handle<Dict>& dict) {
  HandleScope scope;
  Handle<ObjectArray> data(&scope, dict->data());
  Handle<ObjectArray> keys(&scope, newObjectArray(dict->numItems()));
  word num_keys = 0;
  for (word i = 0; i < data->length(); i += Dict::Bucket::kNumPointers) {
    if (Dict::Bucket::isFilled(*data, i)) {
      DCHECK(num_keys < keys->length(), "%ld ! < %ld", num_keys,
             keys->length());
      keys->atPut(num_keys, Dict::Bucket::key(*data, i));
      num_keys++;
    }
  }
  DCHECK(num_keys == keys->length(), "%ld != %ld", num_keys, keys->length());
  return *keys;
}

Object* Runtime::newSet() {
  HandleScope scope;
  Handle<Set> result(&scope, heap()->createSet());
  result->setNumItems(0);
  result->setData(empty_object_array_);
  return *result;
}

bool Runtime::setLookup(const Handle<ObjectArray>& data,
                        const Handle<Object>& key,
                        const Handle<Object>& key_hash, word* index) {
  word start = Set::Bucket::getIndex(*data, *key_hash);
  word current = start;
  word next_free_index = -1;

  // TODO(mpage) - Quadratic probing?
  word length = data->length();
  if (length == 0) {
    *index = -1;
    return false;
  }

  do {
    if (Set::Bucket::hasKey(*data, current, *key)) {
      *index = current;
      return true;
    } else if (next_free_index == -1 &&
               Set::Bucket::isTombstone(*data, current)) {
      next_free_index = current;
    } else if (Set::Bucket::isEmpty(*data, current)) {
      if (next_free_index == -1) {
        next_free_index = current;
      }
      break;
    }
    current = (current + Set::Bucket::kNumPointers) % length;
  } while (current != start);

  *index = next_free_index;

  return false;
}

ObjectArray* Runtime::setGrow(const Handle<ObjectArray>& data) {
  HandleScope scope;
  word new_length = data->length() * kSetGrowthFactor;
  if (new_length == 0) {
    new_length = kInitialSetCapacity * Set::Bucket::kNumPointers;
  }
  Handle<ObjectArray> new_data(&scope, newObjectArray(new_length));
  // Re-insert items
  for (word i = 0; i < data->length(); i += Set::Bucket::kNumPointers) {
    if (Set::Bucket::isEmpty(*data, i) || Set::Bucket::isTombstone(*data, i)) {
      continue;
    }
    Handle<Object> key(&scope, Set::Bucket::key(*data, i));
    Handle<Object> hash(&scope, Set::Bucket::hash(*data, i));
    word index = -1;
    setLookup(new_data, key, hash, &index);
    DCHECK(index != -1, "unexpected index %ld", index);
    Set::Bucket::set(*new_data, index, *hash, *key);
  }
  return *new_data;
}

Object* Runtime::setAddWithHash(const Handle<Set>& set,
                                const Handle<Object>& value,
                                const Handle<Object>& key_hash) {
  HandleScope scope;
  Handle<ObjectArray> data(&scope, set->data());
  word index = -1;
  bool found = setLookup(data, value, key_hash, &index);
  if (found) {
    DCHECK(index != -1, "unexpected index %ld", index);
    return Set::Bucket::key(*data, index);
  }
  if (index == -1) {
    // TODO(mpage): Grow at a predetermined load factor, rather than when full
    Handle<ObjectArray> new_data(&scope, setGrow(data));
    setLookup(new_data, value, key_hash, &index);
    DCHECK(index != -1, "unexpected index %ld", index);
    set->setData(*new_data);
    Set::Bucket::set(*new_data, index, *key_hash, *value);
  } else {
    Set::Bucket::set(*data, index, *key_hash, *value);
  }
  set->setNumItems(set->numItems() + 1);
  return *value;
}

Object* Runtime::setAdd(const Handle<Set>& set, const Handle<Object>& value) {
  HandleScope scope;
  Handle<Object> key_hash(&scope, hash(*value));
  return setAddWithHash(set, value, key_hash);
}

bool Runtime::setIncludes(const Handle<Set>& set, const Handle<Object>& value) {
  HandleScope scope;
  Handle<ObjectArray> data(&scope, set->data());
  Handle<Object> key_hash(&scope, hash(*value));
  word ignore;
  return setLookup(data, value, key_hash, &ignore);
}

bool Runtime::setRemove(const Handle<Set>& set, const Handle<Object>& value) {
  HandleScope scope;
  Handle<ObjectArray> data(&scope, set->data());
  Handle<Object> key_hash(&scope, hash(*value));
  word index = -1;
  bool found = setLookup(data, value, key_hash, &index);
  if (found) {
    DCHECK(index != -1, "unexpected index %ld", index);
    Set::Bucket::setTombstone(*data, index);
    set->setNumItems(set->numItems() - 1);
  }
  return found;
}

Object* Runtime::setUpdate(Thread* thread, const Handle<Set>& dst,
                           const Handle<Object>& iterable) {
  HandleScope scope;
  Handle<Object> elt(&scope, None::object());
  // Special case for lists
  if (iterable->isList()) {
    Handle<List> src(&scope, *iterable);
    for (word i = 0; i < src->allocated(); i++) {
      elt = src->at(i);
      setAdd(dst, elt);
    }
    return *dst;
  }
  // Special case for lists iterators
  if (iterable->isListIterator()) {
    Handle<ListIterator> list_iter(&scope, *iterable);
    Handle<List> src(&scope, list_iter->list());
    for (word i = 0; i < src->allocated(); i++) {
      elt = src->at(i);
      setAdd(dst, elt);
    }
  }
  // Special case for tuples
  if (iterable->isObjectArray()) {
    Handle<ObjectArray> tuple(&scope, *iterable);
    if (tuple->length() > 0) {
      for (word i = 0; i < tuple->length(); i++) {
        elt = tuple->at(i);
        setAdd(dst, elt);
      }
    }
    return *dst;
  }
  // Special case for sets
  if (iterable->isSet()) {
    Handle<Set> src(&scope, *iterable);
    Handle<ObjectArray> data(&scope, src->data());
    if (src->numItems() > 0) {
      for (word i = 0; i < data->length(); i += Set::Bucket::kNumPointers) {
        if (Set::Bucket::isTombstone(*data, i) ||
            Set::Bucket::isEmpty(*data, i)) {
          continue;
        }
        elt = Set::Bucket::key(*data, i);
        setAdd(dst, elt);
      }
    }
    return *dst;
  }
  // Special case for dicts
  if (iterable->isDict()) {
    Handle<Dict> dict(&scope, *iterable);
    if (dict->numItems() > 0) {
      Handle<ObjectArray> keys(&scope, dictKeys(dict));
      Handle<Object> value(&scope, None::object());
      for (word i = 0; i < keys->length(); i++) {
        value = keys->at(i);
        setAdd(dst, value);
      }
    }
    return *dst;
  }
  // Generic case
  Handle<Object> iter_method(
      &scope, Interpreter::lookupMethod(thread, thread->currentFrame(),
                                        iterable, SymbolId::kDunderIter));
  if (iter_method->isError()) {
    return thread->throwTypeErrorFromCStr("object is not iterable");
  }
  Handle<Object> iterator(
      &scope, Interpreter::callMethod1(thread, thread->currentFrame(),
                                       iter_method, iterable));
  if (iterator->isError()) {
    return thread->throwTypeErrorFromCStr("object is not iterable");
  }
  Handle<Object> next_method(
      &scope, Interpreter::lookupMethod(thread, thread->currentFrame(),
                                        iterator, SymbolId::kDunderNext));
  if (next_method->isError()) {
    return thread->throwTypeErrorFromCStr("iter() returned a non-iterator");
  }
  Handle<Object> value(&scope, None::object());
  while (!isIteratorExhausted(thread, iterator)) {
    value = Interpreter::callMethod1(thread, thread->currentFrame(),
                                     next_method, iterator);
    if (value->isError()) {
      return *value;
    }
    setAdd(dst, value);
  }
  return *dst;
}

Object* Runtime::dictUpdate(Thread* thread, const Handle<Dict>& dict,
                            const Handle<Object>& mapping) {
  return dictUpdate<false>(thread, dict, mapping);
}

Object* Runtime::dictMerge(Thread* thread, const Handle<Dict>& dict,
                           const Handle<Object>& mapping) {
  return dictUpdate<true>(thread, dict, mapping);
}

template <bool merge>
inline Object* Runtime::dictUpdate(Thread* thread, const Handle<Dict>& dict,
                                   const Handle<Object>& mapping) {
  HandleScope scope;
  Handle<Object> key(&scope, None::object());
  Handle<Object> value(&scope, None::object());
  if (mapping->isDict()) {
    DCHECK(*mapping != *dict, "Cannot update dict with itself");
    Handle<Dict> other(&scope, *mapping);
    Handle<ObjectArray> data(&scope, other->data());
    for (word i = 0; i < data->length(); i += Dict::Bucket::kNumPointers) {
      if (Dict::Bucket::isFilled(*data, i)) {
        key = Dict::Bucket::key(*data, i);
        value = Dict::Bucket::value(*data, i);
        if (merge) {
          if (!hasSubClassFlag(*key, Type::Flag::kStrSubclass)) {
            return thread->throwTypeErrorFromCStr("keywords must be strings");
          }
          if (dictIncludes(dict, key)) {
            return thread->throwTypeErrorFromCStr(
                "got multiple values for keyword argument");
          }
        }
        dictAtPut(dict, key, value);
      }
    }
    return *dict;
  }
  Frame* frame = thread->currentFrame();
  Handle<Object> keys_method(
      &scope,
      Interpreter::lookupMethod(thread, frame, mapping, SymbolId::kKeys));

  if (keys_method->isError()) {
    return thread->throwTypeErrorFromCStr("object is not a mapping");
  }

  // Generic mapping, use keys() and __getitem__()
  Handle<Object> subscr_method(
      &scope, Interpreter::lookupMethod(thread, frame, mapping,
                                        SymbolId::kDunderGetItem));
  if (subscr_method->isError()) {
    return thread->throwTypeErrorFromCStr("object is not subscriptable");
  }
  Handle<Object> keys(
      &scope, Interpreter::callMethod1(thread, frame, keys_method, mapping));
  if (keys->isList()) {
    Handle<List> keys_list(&scope, *keys);
    for (word i = 0; i < keys_list->allocated(); ++i) {
      key = keys_list->at(i);
      if (merge) {
        if (!hasSubClassFlag(*key, Type::Flag::kStrSubclass)) {
          return thread->throwTypeErrorFromCStr("keywords must be strings");
        }
        if (dictIncludes(dict, key)) {
          return thread->throwTypeErrorFromCStr(
              "got multiple values for keyword argument");
        }
      }
      value =
          Interpreter::callMethod2(thread, frame, subscr_method, mapping, key);
      if (value->isError()) {
        return *value;
      }
      dictAtPut(dict, key, value);
    }
    return *dict;
  }

  if (keys->isObjectArray()) {
    Handle<ObjectArray> keys_tuple(&scope, *keys);
    for (word i = 0; i < keys_tuple->length(); ++i) {
      key = keys_tuple->at(i);
      if (merge) {
        if (!hasSubClassFlag(*key, Type::Flag::kStrSubclass)) {
          return thread->throwTypeErrorFromCStr("keywords must be strings");
        }
        if (dictIncludes(dict, key)) {
          return thread->throwTypeErrorFromCStr(
              "got multiple values for keyword argument");
        }
      }
      value =
          Interpreter::callMethod2(thread, frame, subscr_method, mapping, key);
      if (value->isError()) {
        return *value;
      }
      dictAtPut(dict, key, value);
    }
    return *dict;
  }

  // keys is probably an iterator
  Handle<Object> iter_method(
      &scope, Interpreter::lookupMethod(thread, thread->currentFrame(), keys,
                                        SymbolId::kDunderIter));
  if (iter_method->isError()) {
    return thread->throwTypeErrorFromCStr("o.keys() are not iterable");
  }

  Handle<Object> iterator(
      &scope, Interpreter::callMethod1(thread, thread->currentFrame(),
                                       iter_method, keys));
  if (iterator->isError()) {
    return thread->throwTypeErrorFromCStr("o.keys() are not iterable");
  }
  Handle<Object> next_method(
      &scope, Interpreter::lookupMethod(thread, thread->currentFrame(),
                                        iterator, SymbolId::kDunderNext));
  if (next_method->isError()) {
    thread->throwTypeErrorFromCStr("o.keys() are not iterable");
    thread->abortOnPendingException();
  }
  while (!isIteratorExhausted(thread, iterator)) {
    key = Interpreter::callMethod1(thread, thread->currentFrame(), next_method,
                                   iterator);
    if (key->isError()) {
      return *key;
    }
    if (merge) {
      if (!hasSubClassFlag(*key, Type::Flag::kStrSubclass)) {
        return thread->throwTypeErrorFromCStr("keywords must be strings");
      }
      if (dictIncludes(dict, key)) {
        return thread->throwTypeErrorFromCStr(
            "got multiple values for keyword argument");
      }
    }
    value =
        Interpreter::callMethod2(thread, frame, subscr_method, mapping, key);
    if (value->isError()) {
      return *value;
    }
    dictAtPut(dict, key, value);
  }
  return *dict;
}

Object* Runtime::newValueCell() { return heap()->createValueCell(); }

Object* Runtime::newWeakRef() { return heap()->createWeakRef(); }

void Runtime::collectAttributes(const Handle<Code>& code,
                                const Handle<Dict>& attributes) {
  HandleScope scope;
  Handle<ByteArray> bc(&scope, code->code());
  Handle<ObjectArray> names(&scope, code->names());

  word len = bc->length();
  for (word i = 0; i < len - 3; i += 2) {
    // If the current instruction is EXTENDED_ARG we must skip it and the next
    // instruction.
    if (bc->byteAt(i) == Bytecode::EXTENDED_ARG) {
      i += 2;
      continue;
    }
    // Check for LOAD_FAST 0 (self)
    if (bc->byteAt(i) != Bytecode::LOAD_FAST || bc->byteAt(i + 1) != 0) {
      continue;
    }
    // Followed by a STORE_ATTR
    if (bc->byteAt(i + 2) != Bytecode::STORE_ATTR) {
      continue;
    }
    word name_index = bc->byteAt(i + 3);
    Handle<Object> name(&scope, names->at(name_index));
    dictAtPut(attributes, name, name);
  }
}

Object* Runtime::classConstructor(const Handle<Type>& type) {
  HandleScope scope;
  Handle<Dict> type_dict(&scope, type->dict());
  Handle<Object> init(&scope, symbols()->DunderInit());
  Object* value = dictAt(type_dict, init);
  if (value->isError()) {
    return None::object();
  }
  return ValueCell::cast(value)->value();
}

Object* Runtime::computeInitialLayout(Thread* thread, const Handle<Type>& klass,
                                      LayoutId base_layout_id) {
  HandleScope scope(thread);
  // Create the layout
  LayoutId layout_id = reserveLayoutId();
  Handle<Layout> layout(&scope, layoutCreateSubclassWithBuiltins(
                                    layout_id, base_layout_id,
                                    View<BuiltinAttribute>(nullptr, 0)));

  Handle<ObjectArray> mro(&scope, klass->mro());
  Handle<Dict> attrs(&scope, newDict());

  // Collect set of in-object attributes by scanning the __init__ method of
  // each class in the MRO
  for (word i = 0; i < mro->length(); i++) {
    Handle<Type> mro_type(&scope, mro->at(i));
    Handle<Object> maybe_init(&scope, classConstructor(mro_type));
    if (!maybe_init->isFunction()) {
      continue;
    }
    Handle<Function> init(maybe_init);
    Object* maybe_code = init->code();
    if (!maybe_code->isCode()) {
      continue;
    }
    Handle<Code> code(&scope, maybe_code);
    collectAttributes(code, attrs);
  }

  layout->setNumInObjectAttributes(layout->numInObjectAttributes() +
                                   attrs->numItems());
  layoutAtPut(layout_id, *layout);

  return *layout;
}

Object* Runtime::lookupNameInMro(Thread* thread, const Handle<Type>& type,
                                 const Handle<Object>& name) {
  HandleScope scope(thread);
  Handle<ObjectArray> mro(&scope, type->mro());
  for (word i = 0; i < mro->length(); i++) {
    Handle<Type> mro_type(&scope, mro->at(i));
    Handle<Dict> dict(&scope, mro_type->dict());
    Handle<Object> value_cell(&scope, dictAt(dict, name));
    if (!value_cell->isError()) {
      return ValueCell::cast(*value_cell)->value();
    }
  }
  return Error::object();
}

Object* Runtime::attributeAt(Thread* thread, const Handle<Object>& receiver,
                             const Handle<Object>& name) {
  // A minimal implementation of getattr needed to get richards running.
  Object* result;
  HandleScope scope(thread);
  if (isInstanceOfClass(*receiver)) {
    result = classGetAttr(thread, receiver, name);
  } else if (receiver->isModule()) {
    result = moduleGetAttr(thread, receiver, name);
  } else if (receiver->isSuper()) {
    // TODO(T27518836): remove when we support __getattro__
    result = superGetAttr(thread, receiver, name);
  } else {
    // everything else should fallback to instance
    result = instanceGetAttr(thread, receiver, name);
  }
  return result;
}

Object* Runtime::attributeAtPut(Thread* thread, const Handle<Object>& receiver,
                                const Handle<Object>& name,
                                const Handle<Object>& value) {
  HandleScope scope(thread);
  Handle<Object> interned_name(&scope, internStr(name));
  // A minimal implementation of setattr needed to get richards running.
  Object* result;
  if (isInstanceOfClass(*receiver)) {
    result = classSetAttr(thread, receiver, interned_name, value);
  } else if (receiver->isModule()) {
    result = moduleSetAttr(thread, receiver, interned_name, value);
  } else {
    // everything else should fallback to instance
    result = instanceSetAttr(thread, receiver, interned_name, value);
  }
  return result;
}

Object* Runtime::attributeDel(Thread* thread, const Handle<Object>& receiver,
                              const Handle<Object>& name) {
  HandleScope scope(thread);
  // If present, __delattr__ overrides all attribute deletion logic.
  Handle<Type> type(&scope, typeOf(*receiver));
  Handle<Object> dunder_delattr(
      &scope, lookupSymbolInMro(thread, type, SymbolId::kDunderDelattr));
  Object* result;
  if (!dunder_delattr->isError()) {
    result = Interpreter::callMethod2(thread, thread->currentFrame(),
                                      dunder_delattr, receiver, name);
  } else if (isInstanceOfClass(*receiver)) {
    result = classDelAttr(thread, receiver, name);
  } else if (receiver->isModule()) {
    result = moduleDelAttr(thread, receiver, name);
  } else {
    result = instanceDelAttr(thread, receiver, name);
  }

  return result;
}

Object* Runtime::strConcat(const Handle<Str>& left, const Handle<Str>& right) {
  HandleScope scope;
  word left_len = left->length();
  word right_len = right->length();
  word result_len = left_len + right_len;
  // Small result
  if (result_len <= SmallStr::kMaxLength) {
    byte buffer[SmallStr::kMaxLength];
    left->copyTo(buffer, left_len);
    right->copyTo(buffer + left_len, right_len);
    return SmallStr::fromBytes(View<byte>(buffer, result_len));
  }
  // Large result
  Handle<LargeStr> result(&scope, heap()->createLargeStr(result_len));
  left->copyTo(reinterpret_cast<byte*>(result->address()), left_len);
  right->copyTo(reinterpret_cast<byte*>(result->address() + left_len),
                right_len);
  return *result;
}

Object* Runtime::strJoin(Thread* thread, const Handle<Str>& sep,
                         const Handle<ObjectArray>& items, word allocated) {
  HandleScope scope(thread);
  word result_len = 0;
  for (word i = 0; i < allocated; ++i) {
    Handle<Object> elt(&scope, items->at(i));
    if (!elt->isStr() && !hasSubClassFlag(*elt, Type::Flag::kStrSubclass)) {
      return thread->throwTypeError(
          newStrFromFormat("sequence item %ld: expected str instance", i));
    }
    Handle<Str> str(&scope, items->at(i));
    result_len += str->length();
  }
  if (allocated > 1) {
    result_len += sep->length() * (allocated - 1);
  }
  // Small result
  if (result_len <= SmallStr::kMaxLength) {
    byte buffer[SmallStr::kMaxLength];
    for (word i = 0, offset = 0; i < allocated; ++i) {
      Handle<Str> str(&scope, items->at(i));
      word str_len = str->length();
      str->copyTo(&buffer[offset], str_len);
      offset += str_len;
      if ((i + 1) < allocated) {
        word sep_len = sep->length();
        sep->copyTo(&buffer[offset], sep_len);
        offset += sep->length();
      }
    }
    return SmallStr::fromBytes(View<byte>(buffer, result_len));
  }
  // Large result
  Handle<LargeStr> result(&scope, heap()->createLargeStr(result_len));
  for (word i = 0, offset = 0; i < allocated; ++i) {
    Handle<Str> str(&scope, items->at(i));
    word str_len = str->length();
    str->copyTo(reinterpret_cast<byte*>(result->address() + offset), str_len);
    offset += str_len;
    if ((i + 1) < allocated) {
      word sep_len = sep->length();
      sep->copyTo(reinterpret_cast<byte*>(result->address() + offset), sep_len);
      offset += sep_len;
    }
  }
  return *result;
}

Object* Runtime::computeFastGlobals(const Handle<Code>& code,
                                    const Handle<Dict>& globals,
                                    const Handle<Dict>& builtins) {
  HandleScope scope;
  Handle<ByteArray> bytes(&scope, code->code());
  Handle<ObjectArray> names(&scope, code->names());
  Handle<ObjectArray> fast_globals(&scope, newObjectArray(names->length()));
  for (word i = 0; i < bytes->length(); i += 2) {
    Bytecode bc = static_cast<Bytecode>(bytes->byteAt(i));
    word arg = bytes->byteAt(i + 1);
    while (bc == EXTENDED_ARG) {
      i += 2;
      bc = static_cast<Bytecode>(bytes->byteAt(i));
      arg = (arg << 8) | bytes->byteAt(i + 1);
    }
    if (bc != LOAD_GLOBAL && bc != STORE_GLOBAL && bc != DELETE_GLOBAL) {
      continue;
    }
    Handle<Object> key(&scope, names->at(arg));
    Object* value = dictAt(globals, key);
    if (value->isError()) {
      value = dictAt(builtins, key);
      if (value->isError()) {
        // insert a place holder to allow {STORE|DELETE}_GLOBAL
        Handle<Object> handle(&scope, value);
        value = dictAtPutInValueCell(builtins, key, handle);
        ValueCell::cast(value)->makeUnbound();
      }
      Handle<Object> handle(&scope, value);
      value = dictAtPutInValueCell(globals, key, handle);
    }
    DCHECK(value->isValueCell(), "not  value cell");
    fast_globals->atPut(arg, value);
  }
  return *fast_globals;
}

// See https://github.com/python/cpython/blob/master/Objects/lnotab_notes.txt
// for details about the line number table format
word Runtime::codeOffsetToLineNum(Thread* thread, const Handle<Code>& code,
                                  word offset) {
  HandleScope scope(thread);
  Handle<ByteArray> table(&scope, code->lnotab());
  word line = code->firstlineno();
  word cur_offset = 0;
  for (word i = 0; i < table->length(); i += 2) {
    cur_offset += table->byteAt(i);
    if (cur_offset > offset) {
      break;
    }
    line += static_cast<sbyte>(table->byteAt(i + 1));
  }
  return line;
}

Object* Runtime::isSubClass(const Handle<Type>& subclass,
                            const Handle<Type>& superclass) {
  HandleScope scope;
  Handle<ObjectArray> mro(&scope, subclass->mro());
  for (word i = 0; i < mro->length(); i++) {
    if (mro->at(i) == *superclass) {
      return Bool::trueObj();
    }
  }
  return Bool::falseObj();
}

Object* Runtime::isInstance(const Handle<Object>& obj,
                            const Handle<Type>& klass) {
  HandleScope scope;
  Handle<Type> obj_class(&scope, typeOf(*obj));
  return isSubClass(obj_class, klass);
}

Object* Runtime::newClassMethod() { return heap()->createClassMethod(); }

Object* Runtime::newSuper() { return heap()->createSuper(); }

Object* Runtime::newTupleIterator(const Handle<Object>& tuple) {
  HandleScope scope;
  Handle<TupleIterator> tuple_iterator(&scope, heap()->createTupleIterator());
  tuple_iterator->setTuple(*tuple);
  return *tuple_iterator;
}

LayoutId Runtime::computeBuiltinBaseClass(const Handle<Type>& klass) {
  // The base class can only be one of the builtin bases including object.
  // We use the first non-object builtin base if any, throw if multiple.
  HandleScope scope;
  Handle<ObjectArray> mro(&scope, klass->mro());
  Handle<Type> object_klass(&scope, typeAt(LayoutId::kObject));
  Handle<Type> candidate(&scope, *object_klass);
  // Skip itself since builtin class won't go through this.
  DCHECK(Type::cast(mro->at(0))->instanceLayout()->isNone(),
         "only user defined class should go through this via type_new, and at "
         "this point layout is not ready");
  for (word i = 1; i < mro->length(); i++) {
    Handle<Type> mro_klass(&scope, mro->at(i));
    if (!mro_klass->isIntrinsicOrExtension()) {
      continue;
    }
    if (*candidate == *object_klass) {
      candidate = *mro_klass;
    } else if (*mro_klass != *object_klass &&
               !ObjectArray::cast(candidate->mro())->contains(*mro_klass)) {
      // Allow subclassing of built-in classes that are themselves subclasses
      // of built-in classes (e.g. Exception)

      // TODO: throw TypeError
      CHECK(false, "multiple bases have instance lay-out conflict '%s' '%s'",
            Str::cast(candidate->name())->toCStr(),
            Str::cast(mro_klass->name())->toCStr());
    }
  }
  return Layout::cast(candidate->instanceLayout())->id();
}

Object* Runtime::instanceAt(Thread* thread, const Handle<HeapObject>& instance,
                            const Handle<Object>& name) {
  HandleScope scope(thread);

  // Figure out where the attribute lives in the instance
  Handle<Layout> layout(&scope, layoutAt(instance->layoutId()));
  AttributeInfo info;
  if (!layoutFindAttribute(thread, layout, name, &info)) {
    return Error::object();
  }

  // Retrieve the attribute
  Object* result;
  if (info.isInObject()) {
    result = instance->instanceVariableAt(info.offset());
  } else {
    Handle<ObjectArray> overflow(
        &scope, instance->instanceVariableAt(layout->overflowOffset()));
    result = overflow->at(info.offset());
  }

  return result;
}

Object* Runtime::instanceAtPut(Thread* thread,
                               const Handle<HeapObject>& instance,
                               const Handle<Object>& name,
                               const Handle<Object>& value) {
  HandleScope scope(thread);

  // If the attribute doesn't exist we'll need to transition the layout
  bool has_new_layout_id = false;
  Handle<Layout> layout(&scope, layoutAt(instance->layoutId()));
  AttributeInfo info;
  if (!layoutFindAttribute(thread, layout, name, &info)) {
    // Transition the layout
    layout = layoutAddAttribute(thread, layout, name, 0);
    has_new_layout_id = true;

    bool found = layoutFindAttribute(thread, layout, name, &info);
    CHECK(found, "couldn't find attribute on new layout");
  }

  // Store the attribute
  if (info.isInObject()) {
    instance->instanceVariableAtPut(info.offset(), *value);
  } else {
    // Build the new overflow array
    Handle<ObjectArray> overflow(
        &scope, instance->instanceVariableAt(layout->overflowOffset()));
    Handle<ObjectArray> new_overflow(&scope,
                                     newObjectArray(overflow->length() + 1));
    overflow->copyTo(*new_overflow);
    new_overflow->atPut(info.offset(), *value);
    instance->instanceVariableAtPut(layout->overflowOffset(), *new_overflow);
  }

  if (has_new_layout_id) {
    instance->setHeader(instance->header()->withLayoutId(layout->id()));
  }

  return None::object();
}

Object* Runtime::instanceDel(Thread* thread, const Handle<HeapObject>& instance,
                             const Handle<Object>& name) {
  HandleScope scope(thread);

  // Make the attribute invisible
  Handle<Layout> old_layout(&scope, layoutAt(instance->layoutId()));
  Handle<Object> result(&scope,
                        layoutDeleteAttribute(thread, old_layout, name));
  if (result->isError()) {
    return Error::object();
  }
  LayoutId new_layout_id = Layout::cast(*result)->id();
  instance->setHeader(instance->header()->withLayoutId(new_layout_id));

  // Remove the reference to the attribute value from the instance
  AttributeInfo info;
  bool found = layoutFindAttribute(thread, old_layout, name, &info);
  CHECK(found, "couldn't find attribute");
  if (info.isInObject()) {
    instance->instanceVariableAtPut(info.offset(), None::object());
  } else {
    Handle<ObjectArray> overflow(
        &scope, instance->instanceVariableAt(old_layout->overflowOffset()));
    overflow->atPut(info.offset(), None::object());
  }

  return None::object();
}

Object* Runtime::layoutFollowEdge(const Handle<List>& edges,
                                  const Handle<Object>& label) {
  DCHECK(edges->allocated() % 2 == 0,
         "edges must contain an even number of elements");
  for (word i = 0; i < edges->allocated(); i++) {
    if (edges->at(i) == *label) {
      return edges->at(i + 1);
    }
  }
  return Error::object();
}

void Runtime::layoutAddEdge(const Handle<List>& edges,
                            const Handle<Object>& label,
                            const Handle<Object>& layout) {
  DCHECK(edges->allocated() % 2 == 0,
         "edges must contain an even number of elements");
  listAdd(edges, label);
  listAdd(edges, layout);
}

bool Runtime::layoutFindAttribute(Thread* thread, const Handle<Layout>& layout,
                                  const Handle<Object>& name,
                                  AttributeInfo* info) {
  HandleScope scope(thread);
  Handle<Object> iname(&scope, internStr(name));

  // Check in-object attributes
  Handle<ObjectArray> in_object(&scope, layout->inObjectAttributes());
  for (word i = 0; i < in_object->length(); i++) {
    Handle<ObjectArray> entry(&scope, in_object->at(i));
    if (entry->at(0) == *iname) {
      *info = AttributeInfo(entry->at(1));
      return true;
    }
  }

  // Check overflow attributes
  Handle<ObjectArray> overflow(&scope, layout->overflowAttributes());
  for (word i = 0; i < overflow->length(); i++) {
    Handle<ObjectArray> entry(&scope, overflow->at(i));
    if (entry->at(0) == *iname) {
      *info = AttributeInfo(entry->at(1));
      return true;
    }
  }

  return false;
}

Object* Runtime::layoutCreateEmpty(Thread* thread) {
  HandleScope scope(thread);
  Handle<Layout> result(&scope, newLayout());
  result->setId(reserveLayoutId());
  layoutAtPut(result->id(), *result);
  return *result;
}

Object* Runtime::layoutCreateChild(Thread* thread,
                                   const Handle<Layout>& layout) {
  HandleScope scope(thread);
  Handle<Layout> new_layout(&scope, newLayout());
  new_layout->setId(reserveLayoutId());
  new_layout->setDescribedClass(layout->describedClass());
  new_layout->setNumInObjectAttributes(layout->numInObjectAttributes());
  new_layout->setInObjectAttributes(layout->inObjectAttributes());
  new_layout->setOverflowAttributes(layout->overflowAttributes());
  new_layout->setInstanceSize(layout->instanceSize());
  layoutAtPut(new_layout->id(), *new_layout);
  return *new_layout;
}

Object* Runtime::layoutAddAttributeEntry(Thread* thread,
                                         const Handle<ObjectArray>& entries,
                                         const Handle<Object>& name,
                                         AttributeInfo info) {
  HandleScope scope(thread);
  Handle<ObjectArray> new_entries(&scope,
                                  newObjectArray(entries->length() + 1));
  entries->copyTo(*new_entries);

  Handle<ObjectArray> entry(&scope, newObjectArray(2));
  entry->atPut(0, *name);
  entry->atPut(1, info.asSmallInt());
  new_entries->atPut(entries->length(), *entry);

  return *new_entries;
}

Object* Runtime::layoutAddAttribute(Thread* thread,
                                    const Handle<Layout>& layout,
                                    const Handle<Object>& name, word flags) {
  HandleScope scope(thread);
  Handle<Object> iname(&scope, internStr(name));

  // Check if a edge for the attribute addition already exists
  Handle<List> edges(&scope, layout->additions());
  Object* result = layoutFollowEdge(edges, iname);
  if (!result->isError()) {
    return result;
  }

  // Create a new layout and figure out where to place the attribute
  Handle<Layout> new_layout(&scope, layoutCreateChild(thread, layout));
  Handle<ObjectArray> inobject(&scope, layout->inObjectAttributes());
  if (inobject->length() < layout->numInObjectAttributes()) {
    AttributeInfo info(inobject->length() * kPointerSize,
                       flags | AttributeInfo::Flag::kInObject);
    new_layout->setInObjectAttributes(
        layoutAddAttributeEntry(thread, inobject, name, info));
  } else {
    Handle<ObjectArray> overflow(&scope, layout->overflowAttributes());
    AttributeInfo info(overflow->length(), flags);
    new_layout->setOverflowAttributes(
        layoutAddAttributeEntry(thread, overflow, name, info));
  }

  // Add the edge to the existing layout
  Handle<Object> value(&scope, *new_layout);
  layoutAddEdge(edges, iname, value);

  return *new_layout;
}

Object* Runtime::layoutDeleteAttribute(Thread* thread,
                                       const Handle<Layout>& layout,
                                       const Handle<Object>& name) {
  HandleScope scope(thread);

  // See if the attribute exists
  AttributeInfo info;
  if (!layoutFindAttribute(thread, layout, name, &info)) {
    return Error::object();
  }

  // Check if an edge exists for removing the attribute
  Handle<Object> iname(&scope, internStr(name));
  Handle<List> edges(&scope, layout->deletions());
  Object* next_layout = layoutFollowEdge(edges, iname);
  if (!next_layout->isError()) {
    return next_layout;
  }

  // No edge was found, create a new layout and add an edge
  Handle<Layout> new_layout(&scope, layoutCreateChild(thread, layout));
  if (info.isInObject()) {
    // The attribute to be deleted was an in-object attribute, mark it as
    // deleted
    Handle<ObjectArray> old_inobject(&scope, layout->inObjectAttributes());
    Handle<ObjectArray> new_inobject(&scope,
                                     newObjectArray(old_inobject->length()));
    for (word i = 0; i < old_inobject->length(); i++) {
      Handle<ObjectArray> entry(&scope, old_inobject->at(i));
      if (entry->at(0) == *iname) {
        entry = newObjectArray(2);
        entry->atPut(0, None::object());
        entry->atPut(
            1, AttributeInfo(0, AttributeInfo::Flag::kDeleted).asSmallInt());
      }
      new_inobject->atPut(i, *entry);
    }
    new_layout->setInObjectAttributes(*new_inobject);
  } else {
    // The attribute to be deleted was an overflow attribute, omit it from the
    // new overflow array
    Handle<ObjectArray> old_overflow(&scope, layout->overflowAttributes());
    Handle<ObjectArray> new_overflow(
        &scope, newObjectArray(old_overflow->length() - 1));
    bool is_deleted = false;
    for (word i = 0, j = 0; i < old_overflow->length(); i++) {
      Handle<ObjectArray> entry(&scope, old_overflow->at(i));
      if (entry->at(0) == *iname) {
        is_deleted = true;
        continue;
      }
      if (is_deleted) {
        // Need to shift everything down by 1 once we've deleted the attribute
        entry = newObjectArray(2);
        entry->atPut(0, ObjectArray::cast(old_overflow->at(i))->at(0));
        entry->atPut(1, AttributeInfo(j, info.flags()).asSmallInt());
      }
      new_overflow->atPut(j, *entry);
      j++;
    }
    new_layout->setOverflowAttributes(*new_overflow);
  }

  // Add the edge to the existing layout
  Handle<Object> value(&scope, *new_layout);
  layoutAddEdge(edges, iname, value);

  return *new_layout;
}

void Runtime::initializeSuperClass() {
  HandleScope scope;
  Handle<Type> super(&scope,
                     addEmptyBuiltinClass(SymbolId::kSuper, LayoutId::kSuper,
                                          LayoutId::kObject));

  classAddBuiltinFunction(super, SymbolId::kDunderInit,
                          nativeTrampoline<builtinSuperInit>);

  classAddBuiltinFunction(super, SymbolId::kDunderNew,
                          nativeTrampoline<builtinSuperNew>);
}

Object* Runtime::superGetAttr(Thread* thread, const Handle<Object>& receiver,
                              const Handle<Object>& name) {
  HandleScope scope(thread);
  Handle<Super> super(&scope, *receiver);
  Handle<Type> start_type(&scope, super->objectType());
  Handle<ObjectArray> mro(&scope, start_type->mro());
  word i;
  for (i = 0; i < mro->length(); i++) {
    if (super->type() == mro->at(i)) {
      // skip super->type (if any)
      i++;
      break;
    }
  }
  for (; i < mro->length(); i++) {
    Handle<Type> type(&scope, mro->at(i));
    Handle<Dict> dict(&scope, type->dict());
    Handle<Object> value_cell(&scope, dictAt(dict, name));
    if (value_cell->isError()) {
      continue;
    }
    Handle<Object> value(&scope, ValueCell::cast(*value_cell)->value());
    if (!isNonDataDescriptor(thread, value)) {
      return *value;
    } else {
      Handle<Object> self(&scope, None::object());
      if (super->object() != *start_type) {
        self = super->object();
      }
      Handle<Object> owner(&scope, *start_type);
      return Interpreter::callDescriptorGet(thread, thread->currentFrame(),
                                            value, self, owner);
    }
  }
  // fallback to normal instance getattr
  return instanceGetAttr(thread, receiver, name);
}

void Runtime::freeApiHandles() {
  // Clear the allocated ApiHandles
  HandleScope scope;
  Handle<Dict> dict(&scope, apiHandles());
  Handle<ObjectArray> keys(&scope, dictKeys(dict));
  for (word i = 0; i < keys->length(); i++) {
    Handle<Object> key(&scope, keys->at(i));
    Object* value = dictAt(dict, key);
    delete static_cast<ApiHandle*>(Int::cast(value)->asCPointer());
  }
}

Object* Runtime::lookupSymbolInMro(Thread* thread, const Handle<Type>& type,
                                   SymbolId symbol) {
  HandleScope scope(thread);
  Handle<ObjectArray> mro(&scope, type->mro());
  Handle<Object> key(&scope, symbols()->at(symbol));
  for (word i = 0; i < mro->length(); i++) {
    Handle<Type> mro_type(&scope, mro->at(i));
    Handle<Dict> dict(&scope, mro_type->dict());
    Handle<Object> value_cell(&scope, dictAt(dict, key));
    if (!value_cell->isError()) {
      return ValueCell::cast(*value_cell)->value();
    }
  }
  return Error::object();
}

bool Runtime::isIteratorExhausted(Thread* thread,
                                  const Handle<Object>& iterator) {
  HandleScope scope(thread);
  Handle<Object> length_hint_method(
      &scope, Interpreter::lookupMethod(thread, thread->currentFrame(),
                                        iterator, SymbolId::kDunderLengthHint));
  if (length_hint_method->isError()) {
    return true;
  }
  Handle<Object> result(&scope,
                        Interpreter::callMethod1(thread, thread->currentFrame(),
                                                 length_hint_method, iterator));
  if (result->isError()) {
    return true;
  }
  if (!result->isSmallInt()) {
    return true;
  }
  return (SmallInt::cast(*result)->value() == 0);
}

}  // namespace python

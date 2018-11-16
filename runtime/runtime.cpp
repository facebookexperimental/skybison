#include "runtime.h"

#include <cstring>

#include "builtins.h"
#include "callback.h"
#include "globals.h"
#include "handles.h"
#include "heap.h"
#include "marshal.h"
#include "os.h"
#include "siphash.h"
#include "thread.h"
#include "trampolines.h"
#include "visitor.h"

namespace python {

Runtime::Runtime() : heap_(64 * MiB), new_value_cell_callback_(this) {
  initializeRandom();
  initializeThreads();
  initializeClasses();
  initializeInstances();
  initializeInterned();
  initializeModules();
}

Runtime::~Runtime() {
  for (Thread* thread = threads_; thread != nullptr; thread = thread->next()) {
    if (thread == Thread::currentThread()) {
      Thread::setCurrentThread(nullptr);
    } else {
      assert(0); // Not implemented.
    }
    delete thread;
  }
}

Object* Runtime::newByteArray(word length) {
  if (length == 0) {
    return empty_byte_array_;
  }
  return heap()->createByteArray(length);
}

Object* Runtime::newCode() {
  return heap()->createCode(empty_object_array_);
}

Object* Runtime::newByteArrayWithAll(const byte* data, word length) {
  if (length == 0) {
    return empty_byte_array_;
  }
  Object* result = newByteArray(length);
  for (word i = 0; i < length; i++) {
    ByteArray::cast(result)->byteAtPut(
        i, *reinterpret_cast<const byte*>(data + i));
  }
  return result;
}

Object* Runtime::newBuiltinFunction(Function::Entry entry) {
  Object* result = heap()->createFunction();
  assert(result != nullptr);
  auto function = Function::cast(result);
  function->setEntry(entry);
  function->setEntryKw(unimplementedTrampoline);
  return result;
}

Object* Runtime::newFunction() {
  Object* object = heap()->createFunction();
  assert(object != nullptr);
  auto function = Function::cast(object);
  function->setEntry(unimplementedTrampoline);
  function->setEntryKw(unimplementedTrampoline);
  return function;
}

Object* Runtime::newList() {
  return heap()->createList(empty_object_array_);
}

Object* Runtime::newModule(const Handle<Object>& name) {
  HandleScope scope;
  Handle<Dictionary> dictionary(&scope, newDictionary());
  Handle<Object> key(&scope, newStringFromCString("__name__"));
  dictionaryAtPut(dictionary, key, name);
  return heap()->createModule(*name, *dictionary);
}

Object* Runtime::newObjectArray(word length) {
  if (length == 0) {
    return empty_object_array_;
  }
  return heap()->createObjectArray(length, None::object());
}

Object* Runtime::newString(word length) {
  if (length == 0) {
    return empty_string_;
  }
  return heap()->createString(length);
}

Object* Runtime::newStringFromCString(const char* c_string) {
  word length = std::strlen(c_string);
  if (length == 0) {
    return empty_string_;
  }
  Object* result = newString(length);
  assert(result != nullptr);
  for (word i = 0; i < length; i++) {
    String::cast(result)->charAtPut(
        i, *reinterpret_cast<const byte*>(c_string + i));
  }
  return result;
}

Object* Runtime::internString(const Handle<Object>& string) {
  HandleScope scope;
  Handle<Dictionary> dictionary(&scope, interned());
  Handle<Object> key(&scope, *string);
  Object* value;
  if (dictionaryAt(dictionary, key, &value)) {
    return value;
  }
  dictionaryAtPut(dictionary, key, key);
  return *string;
}

Object* Runtime::hash(Object* object) {
  if (!object->isHeapObject()) {
    return immediateHash(object);
  }
  if (object->isByteArray() || object->isString()) {
    return valueHash(object);
  }
  return identityHash(object);
}

Object* Runtime::immediateHash(Object* object) {
  if (object->isSmallInteger()) {
    return object;
  }
  if (object->isBoolean()) {
    return SmallInteger::fromWord(Boolean::cast(object)->value() ? 1 : 0);
  }
  return SmallInteger::fromWord(reinterpret_cast<uword>(object));
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

Object* Runtime::identityHash(Object* object) {
  HeapObject* src = HeapObject::cast(object);
  word code = src->header()->hashCode();
  if (code == 0) {
    code = random();
    code = (code == 0) ? 1 : code;
    src->setHeader(src->header()->withHashCode(code));
  }
  return SmallInteger::fromWord(code);
}

word Runtime::siphash24(const byte* src, word size) {
  word result = 0;
  ::halfsiphash(
      src,
      size,
      reinterpret_cast<const uint8_t*>(hash_secret_),
      reinterpret_cast<uint8_t*>(&result),
      sizeof(result));
  return result;
}

Object* Runtime::valueHash(Object* object) {
  HeapObject* src = HeapObject::cast(object);
  Header* header = src->header();
  word code = header->hashCode();
  if (code == 0) {
    word size = src->headerCountOrOverflow();
    code = siphash24(reinterpret_cast<const byte*>(src->address()), size);
    code &= Header::kHashCodeMask;
    code = (code == 0) ? 1 : code;
    src->setHeader(header->withHashCode(code));
    assert(code == src->header()->hashCode());
  }
  return SmallInteger::fromWord(code);
}

void Runtime::initializeClasses() {
  class_class_ = heap()->createClassClass();

  byte_array_class_ = heap()->createClass(ClassId::kByteArray, class_class_);
  code_class_ = heap()->createClass(ClassId::kCode, class_class_);
  dictionary_class_ = heap()->createClass(ClassId::kDictionary, class_class_);
  function_class_ = heap()->createClass(ClassId::kFunction, class_class_);
  list_class_ = heap()->createClass(ClassId::kList, class_class_);
  module_class_ = heap()->createClass(ClassId::kModule, class_class_);
  object_array_class_ =
      heap()->createClass(ClassId::kObjectArray, class_class_);
  string_class_ = heap()->createClass(ClassId::kString, class_class_);
}

class ScavengeVisitor : public PointerVisitor {
 public:
  explicit ScavengeVisitor(Heap* heap) : heap_(heap) {}

  void visitPointer(Object** pointer) override {
    heap_->scavengePointer(pointer);
  }

 private:
  Heap* heap_;
};

void Runtime::collectGarbage() {
  heap()->flip();
  ScavengeVisitor visitor(heap());
  visitRoots(&visitor);
  heap()->scavenge();
}

Object* Runtime::run(const char* buffer) {
  HandleScope scope;

  Marshal::Reader reader(&scope, this, buffer);

  reader.readLong();
  reader.readLong();
  reader.readLong();

  auto code = reader.readObject();
  assert(code->isCode());
  assert(python::Code::cast(code)->argcount() == 0);

  Handle<Module> main(&scope, createMainModule());
  return python::Thread::currentThread()->runModuleFunction(*main, code);
}

void Runtime::initializeThreads() {
  auto main_thread = new Thread(Thread::kDefaultStackSize);
  threads_ = main_thread;
  main_thread->setRuntime(this);
  Thread::setCurrentThread(main_thread);
}

void Runtime::initializeInstances() {
  empty_byte_array_ = heap()->createByteArray(0);
  empty_object_array_ = heap()->createObjectArray(0, None::object());
  empty_string_ = heap()->createString(0);
}

void Runtime::initializeInterned() {
  interned_ = newDictionary();
}

void Runtime::initializeRandom() {
  Os::secureRandom(
      reinterpret_cast<byte*>(&random_state_), sizeof(random_state_));
  Os::secureRandom(
      reinterpret_cast<byte*>(&hash_secret_), sizeof(hash_secret_));
}

void Runtime::visitRoots(PointerVisitor* visitor) {
  visitRuntimeRoots(visitor);
  visitThreadRoots(visitor);
}

void Runtime::visitRuntimeRoots(PointerVisitor* visitor) {
  // Visit classes
  visitor->visitPointer(&byte_array_class_);
  visitor->visitPointer(&class_class_);
  visitor->visitPointer(&code_class_);
  visitor->visitPointer(&dictionary_class_);
  visitor->visitPointer(&function_class_);
  visitor->visitPointer(&list_class_);
  visitor->visitPointer(&module_class_);
  visitor->visitPointer(&object_array_class_);
  visitor->visitPointer(&string_class_);

  // Visit instances
  visitor->visitPointer(&empty_byte_array_);
  visitor->visitPointer(&empty_object_array_);
  visitor->visitPointer(&empty_string_);

  // Visit interned strings.
  visitor->visitPointer(&interned_);

  // Visit modules
  visitor->visitPointer(&modules_);
}

void Runtime::visitThreadRoots(PointerVisitor* visitor) {
  for (Thread* thread = threads_; thread != nullptr; thread = thread->next()) {
    thread->handles()->visitPointers(visitor);
  }
}

void Runtime::addModule(const Handle<Module>& module) {
  HandleScope scope;
  Handle<Dictionary> dict(&scope, modules());
  Handle<Object> key(&scope, module->name());
  Handle<Object> value(&scope, *module);
  dictionaryAtPut(dict, key, value);
}

void Runtime::initializeModules() {
  modules_ = newDictionary();
  createBuiltinsModule();
  createSysModule();
}

void Runtime::addModuleGlobal(
    const Handle<Module>& module,
    const Handle<Object>& key,
    const Handle<Object>& value) {
  HandleScope scope;
  Handle<Dictionary> dictionary(&scope, module->dictionary());
  Handle<ValueCell> value_cell(
      &scope, dictionaryAtIfAbsentPut(dictionary, key, newValueCellCallback()));
  value_cell->setValue(*value);
}

void Runtime::createBuiltinsModule() {
  HandleScope scope;
  Handle<Object> name(&scope, newStringFromCString("builtins"));
  Handle<Module> module(&scope, newModule(name));

  // Fill in builtins...
  Handle<Object> key(&scope, newStringFromCString("print"));
  Handle<Object> value(&scope, newBuiltinFunction(&builtinPrint));
  addModuleGlobal(module, key, value);

  addModule(module);
}

void Runtime::createSysModule() {
  HandleScope scope;
  Handle<Object> name(&scope, newStringFromCString("sys"));
  Handle<Module> module(&scope, newModule(name));
  // Fill in sys...
  addModule(module);
}

Object* Runtime::createMainModule() {
  HandleScope scope;
  Handle<Object> name(&scope, newStringFromCString("__main__"));
  Handle<Module> module(&scope, newModule(name));

  // Fill in __main__...
  // TODO(cshapiro): inherit all symbols present in builtins.
  Handle<Object> key(&scope, newStringFromCString("print"));
  Handle<Object> value(&scope, newBuiltinFunction(&builtinPrint));
  addModuleGlobal(module, key, value);

  addModule(module);

  return *module;
}

// List

ObjectArray* Runtime::ensureCapacity(
    const Handle<ObjectArray>& array,
    word index) {
  HandleScope scope;
  word capacity = array->length();
  if (index < capacity) {
    return *array;
  }
  word newCapacity = (capacity == 0) ? kInitialEnsuredCapacity : capacity << 1;
  Handle<ObjectArray> newArray(&scope, newObjectArray(newCapacity));
  array->copyTo(*newArray);
  return *newArray;
}

void Runtime::appendToList(
    const Handle<List>& list,
    const Handle<Object>& value) {
  HandleScope scope;
  word index = list->allocated();
  Handle<ObjectArray> items(&scope, list->items());
  Handle<ObjectArray> newItems(&scope, ensureCapacity(items, index));
  if (*items != *newItems) {
    list->setItems(*newItems);
  }
  list->setAllocated(index + 1);
  list->atPut(index, *value);
}

// Dictionary

// Helper class for manipulating buckets in the ObjectArray that backs the
// dictionary
class Bucket {
 public:
  Bucket(const Handle<ObjectArray>& data, word index)
      : data_(data), index_(index) {}

  inline Object* hash() {
    return data_->at(index_ + kHashOffset);
  }

  inline Object* key() {
    return data_->at(index_ + kKeyOffset);
  }

  inline Object* value() {
    return data_->at(index_ + kValueOffset);
  }

  inline void set(Object* hash, Object* key, Object* value) {
    data_->atPut(index_ + kHashOffset, hash);
    data_->atPut(index_ + kKeyOffset, key);
    data_->atPut(index_ + kValueOffset, value);
  }

  inline bool hasKey(Object* thatKey) {
    return !hash()->isNone() && Object::equals(key(), thatKey);
  }

  inline bool isTombstone() {
    return hash()->isNone() && !key()->isNone();
  }

  inline void setTombstone() {
    set(None::object(), Ellipsis::object(), None::object());
  }

  inline bool isEmpty() {
    return hash()->isNone() && key()->isNone();
  }

  static inline word getIndex(Object* data, Object* hash) {
    word nbuckets = ObjectArray::cast(data)->length() / kNumPointers;
    assert(Utils::isPowerOfTwo(nbuckets));
    word value = SmallInteger::cast(hash)->value();
    return (value & (nbuckets - 1)) * kNumPointers;
  }

  static const word kHashOffset = 0;
  static const word kKeyOffset = kHashOffset + 1;
  static const word kValueOffset = kKeyOffset + 1;
  static const word kNumPointers = kValueOffset + 1;

 private:
  const Handle<ObjectArray>& data_;
  word index_;

  DISALLOW_HEAP_ALLOCATION();
};

Object* Runtime::newDictionary() {
  return heap()->createDictionary(empty_object_array_);
}

void Runtime::dictionaryAtPut(
    const Handle<Dictionary>& dict,
    const Handle<Object>& key,
    const Handle<Object>& value) {
  HandleScope scope;
  Handle<ObjectArray> data(&scope, dict->data());
  word index = -1;
  Handle<Object> key_hash(&scope, hash(*key));
  bool found = dictionaryLookup(data, key, key_hash, &index);
  if (index == -1) {
    // TODO(mpage): Grow at a predetermined load factor, rather than when full
    Handle<ObjectArray> newData(&scope, dictionaryGrow(data));
    dictionaryLookup(newData, key, key_hash, &index);
    assert(index != -1);
    dict->setData(*newData);
    Bucket bucket(newData, index);
    bucket.set(*key_hash, *key, *value);
  } else {
    Bucket bucket(data, index);
    bucket.set(*key_hash, *key, *value);
  }
  if (!found) {
    dict->setNumItems(dict->numItems() + 1);
  }
}

ObjectArray* Runtime::dictionaryGrow(const Handle<ObjectArray>& data) {
  HandleScope scope;
  word newLength = data->length() * kDictionaryGrowthFactor;
  if (newLength == 0) {
    newLength = kInitialDictionaryCapacity * Bucket::kNumPointers;
  }
  Handle<ObjectArray> newData(&scope, newObjectArray(newLength));
  // Re-insert items
  for (word i = 0; i < data->length(); i += Bucket::kNumPointers) {
    Bucket oldBucket(data, i);
    if (oldBucket.isEmpty() || oldBucket.isTombstone()) {
      continue;
    }
    Handle<Object> key(&scope, oldBucket.key());
    Handle<Object> hash(&scope, oldBucket.hash());
    word index = -1;
    dictionaryLookup(newData, key, hash, &index);
    assert(index != -1);
    Bucket newBucket(newData, index);
    newBucket.set(*key, *hash, oldBucket.value());
  }
  return *newData;
}

bool Runtime::dictionaryAt(
    const Handle<Dictionary>& dict,
    const Handle<Object>& key,
    Object** value) {
  HandleScope scope;
  Handle<ObjectArray> data(&scope, dict->data());
  word index = -1;
  Handle<Object> key_hash(&scope, hash(*key));
  bool found = dictionaryLookup(data, key, key_hash, &index);
  if (found) {
    assert(index != -1);
    *value = Bucket(data, index).value();
  }
  return found;
}

Object* Runtime::dictionaryAtIfAbsentPut(
    const Handle<Dictionary>& dict,
    const Handle<Object>& key,
    Callback<Object*>* thunk) {
  HandleScope scope;
  Handle<ObjectArray> data(&scope, dict->data());
  word index = -1;
  Handle<Object> key_hash(&scope, hash(*key));
  bool found = dictionaryLookup(data, key, key_hash, &index);
  if (found) {
    assert(index != -1);
    return Bucket(data, index).value();
  }
  Handle<Object> value(&scope, thunk->call());
  if (index == -1) {
    // TODO(mpage): Grow at a predetermined load factor, rather than when full
    Handle<ObjectArray> new_data(&scope, dictionaryGrow(data));
    dictionaryLookup(new_data, key, key_hash, &index);
    assert(index != -1);
    dict->setData(*new_data);
    Bucket bucket(new_data, index);
    bucket.set(*key_hash, *key, *value);
  } else {
    Bucket bucket(data, index);
    bucket.set(*key_hash, *key, *value);
  }
  return *value;
}

bool Runtime::dictionaryRemove(
    const Handle<Dictionary>& dict,
    const Handle<Object>& key,
    Object** value) {
  HandleScope scope;
  Handle<ObjectArray> data(&scope, dict->data());
  word index = -1;
  Handle<Object> key_hash(&scope, hash(*key));
  bool found = dictionaryLookup(data, key, key_hash, &index);
  if (found) {
    assert(index != -1);
    Bucket bucket(data, index);
    *value = bucket.value();
    bucket.setTombstone();
    dict->setNumItems(dict->numItems() - 1);
  }
  return found;
}

bool Runtime::dictionaryLookup(
    const Handle<ObjectArray>& data,
    const Handle<Object>& key,
    const Handle<Object>& key_hash,
    word* index) {
  word start = Bucket::getIndex(*data, *key_hash);
  word current = start;
  word nextFreeIndex = -1;

  // TODO(mpage) - Quadratic probing?
  word length = data->length();
  if (length == 0) {
    *index = -1;
    return false;
  }

  do {
    Bucket bucket(data, current);
    if (bucket.hasKey(*key)) {
      *index = current;
      return true;
    } else if (nextFreeIndex == -1 && bucket.isTombstone()) {
      nextFreeIndex = current;
    } else if (bucket.isEmpty()) {
      if (nextFreeIndex == -1) {
        nextFreeIndex = current;
      }
      break;
    }
    current = (current + Bucket::kNumPointers) % length;
  } while (current != start);

  *index = nextFreeIndex;

  return false;
}

Object* Runtime::newValueCell() {
  return heap()->createValueCell();
}

bool Runtime::isTruthy(Object* object) {
  if (object->isBoolean()) {
    return Boolean::cast(object)->value();
  }
  // TODO(mpage): Raise an exception
  std::abort();
}

} // namespace python

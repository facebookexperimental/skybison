#include "cpython-func.h"

#include "builtins-module.h"
#include "capi-handles.h"
#include "dict-builtins.h"
#include "module-builtins.h"
#include "object-builtins.h"
#include "objects.h"
#include "runtime.h"
#include "type-builtins.h"

char* PyStructSequence_UnnamedField = const_cast<char*>("unnamed field");

namespace py {

PY_EXPORT PyObject* PyStructSequence_GetItem(PyObject* structseq,
                                             Py_ssize_t pos) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  UserTupleBase user_tuple(&scope,
                           ApiHandle::fromPyObject(structseq)->asObject());
  Tuple tuple(&scope, user_tuple.value());
  word num_in_sequence = tuple.length();
  word user_tuple_fields = (UserTupleBase::kSize / kPointerSize);
  word num_fields =
      num_in_sequence + user_tuple.headerCountOrOverflow() - user_tuple_fields;
  CHECK_INDEX(pos, num_fields);
  if (pos < num_in_sequence) {
    return ApiHandle::borrowedReference(thread, tuple.at(pos));
  }
  word offset = (pos - num_in_sequence + user_tuple_fields) * kPointerSize;
  return ApiHandle::borrowedReference(thread,
                                      user_tuple.instanceVariableAt(offset));
}

PY_EXPORT PyObject* PyStructSequence_SET_ITEM_Func(PyObject* structseq,
                                                   Py_ssize_t pos,
                                                   PyObject* value) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  UserTupleBase user_tuple(&scope,
                           ApiHandle::fromPyObject(structseq)->asObject());
  Tuple tuple(&scope, user_tuple.value());
  Object value_obj(&scope, value == nullptr
                               ? NoneType::object()
                               : ApiHandle::stealReference(thread, value));
  word num_in_sequence = tuple.length();
  word user_tuple_fields = (RawUserTupleBase::kSize / kPointerSize);
  word num_fields =
      num_in_sequence + user_tuple.headerCountOrOverflow() - user_tuple_fields;
  CHECK_INDEX(pos, num_fields);
  if (pos < num_in_sequence) {
    tuple.atPut(pos, *value_obj);
  } else {
    word offset = (pos - num_in_sequence + user_tuple_fields) * kPointerSize;
    user_tuple.instanceVariableAtPut(offset, *value_obj);
  }
  return value;
}

PY_EXPORT void PyStructSequence_SetItem(PyObject* structseq, Py_ssize_t pos,
                                        PyObject* value) {
  PyStructSequence_SET_ITEM_Func(structseq, pos, value);
}

PY_EXPORT PyObject* PyStructSequence_New(PyTypeObject* pytype) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Type type(&scope, ApiHandle::fromPyTypeObject(pytype)->asObject());
  Layout layout(&scope, type.instanceLayout());
  UserTupleBase result(&scope, runtime->newInstance(layout));
  Int n_sequence_fields(&scope,
                        typeAtById(thread, type, ID(n_sequence_fields)));
  result.setValue(runtime->newTuple(n_sequence_fields.asWord()));
  return ApiHandle::newReference(thread, *result);
}

PY_EXPORT PyTypeObject* PyStructSequence_NewType(PyStructSequence_Desc* desc) {
  Thread* thread = Thread::current();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  // Create the class name
  const char* class_name = strrchr(desc->name, '.');
  if (class_name == nullptr) {
    class_name = desc->name;
  } else {
    class_name++;
  }
  Str name(&scope, runtime->newStrFromCStr(class_name));

  // Add n_sequence_fields
  Dict dict(&scope, runtime->newDict());
  Object n_sequence(&scope, runtime->newInt(desc->n_in_sequence));
  dictAtPutById(thread, dict, ID(n_sequence_fields), n_sequence);

  // Add n_fields
  word num_fields = 0;
  while (desc->fields[num_fields].name != nullptr) {
    CHECK(desc->fields[num_fields].name != PyStructSequence_UnnamedField,
          "The use of unnamed fields is not allowed");
    num_fields++;
  }
  Object n_fields(&scope, runtime->newInt(num_fields));
  dictAtPutById(thread, dict, ID(n_fields), n_fields);

  // unnamed fields are banned. This is done to support _structseq_getitem(),
  // which accesses hidden fields by name.
  Object unnamed_fields(&scope, runtime->newInt(0));
  dictAtPutById(thread, dict, ID(n_unnamed_fields), unnamed_fields);

  // Add __new__
  Module builtins(&scope, runtime->findModuleById(ID(builtins)));
  Function structseq_new(&scope,
                         moduleAtById(thread, builtins, ID(_structseq_new)));
  dictAtPutById(thread, dict, ID(__new__), structseq_new);

  // Add __repr__
  Function structseq_repr(&scope,
                          moduleAtById(thread, builtins, ID(_structseq_repr)));
  dictAtPutById(thread, dict, ID(__repr__), structseq_repr);

  Tuple field_names(&scope, runtime->newTuple(num_fields));
  for (word i = 0; i < num_fields; i++) {
    field_names.atPut(i,
                      Runtime::internStrFromCStr(thread, desc->fields[i].name));
  }
  dictAtPutById(thread, dict, ID(_structseq_field_names), field_names);

  // Create type
  Tuple bases(&scope, runtime->newTuple(1));
  bases.atPut(0, runtime->typeAt(LayoutId::kTuple));
  Type type(&scope, typeNew(thread, LayoutId::kType, name, bases, dict,
                            static_cast<Type::Flag>(0)));

  // Add hidden fields as in-object attributes in the instance layout.
  Layout layout(&scope, type.instanceLayout());
  if (num_fields > desc->n_in_sequence) {
    Str field_name(&scope, Str::empty());
    for (word i = desc->n_in_sequence, offset = RawUserTupleBase::kSize;
         i < num_fields; i++, offset += kPointerSize) {
      AttributeInfo info(offset, AttributeFlags::kInObject);
      Tuple entries(&scope, layout.inObjectAttributes());
      field_name = field_names.at(i);
      layout.setNumInObjectAttributes(layout.numInObjectAttributes() + 1);
      layout.setInObjectAttributes(
          runtime->layoutAddAttributeEntry(thread, entries, field_name, info));
    }
  }
  layout.seal();

  // Add descriptors for all fields
  Str field_name(&scope, Str::empty());
  Object index(&scope, NoneType::object());
  Object field(&scope, NoneType::object());
  for (word i = 0; i < num_fields; i++) {
    field_name = field_names.at(i);
    index = i < desc->n_in_sequence ? runtime->newInt(i) : NoneType::object();
    field = thread->invokeFunction2(ID(builtins), ID(_structseq_field),
                                    field_name, index);
    typeSetAttr(thread, type, field_name, field);
  }

  return reinterpret_cast<PyTypeObject*>(
      ApiHandle::newReference(thread, *type));
}

}  // namespace py

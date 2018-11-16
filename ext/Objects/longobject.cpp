// longobject.c implementation

#include "handles.h"
#include "objects.h"
#include "runtime.h"

namespace python {

PY_EXPORT int PyLong_CheckExact_Func(PyObject* obj) {
  return ApiHandle::fromPyObject(obj)->asObject()->isInt();
}

PY_EXPORT int PyLong_Check_Func(PyObject* obj) {
  if (PyLong_CheckExact_Func(obj)) {
    return true;
  }
  return ApiHandle::fromPyObject(obj)->isSubClass(Thread::currentThread(),
                                                  LayoutId::kInt);
}

// Converting from signed ints.

PY_EXPORT PyObject* PyLong_FromLong(long ival) {
  return ApiHandle::fromObject(
      Thread::currentThread()->runtime()->newInt(ival));
}

PY_EXPORT PyObject* PyLong_FromLongLong(long long ival) {
  static_assert(sizeof(ival) <= sizeof(long), "Unsupported long long size");
  return PyLong_FromLong(ival);
}

PY_EXPORT PyObject* PyLong_FromSsize_t(Py_ssize_t ival) {
  static_assert(sizeof(ival) <= sizeof(long), "Unsupported Py_ssize_t size");
  return PyLong_FromLong(ival);
}

// Converting from unsigned ints.

PY_EXPORT PyObject* PyLong_FromUnsignedLong(unsigned long ival) {
  static_assert(sizeof(ival) <= sizeof(uword),
                "Unsupported unsigned long type");
  return ApiHandle::fromObject(
      Thread::currentThread()->runtime()->newIntFromUnsigned(ival));
}

PY_EXPORT PyObject* PyLong_FromUnsignedLongLong(unsigned long long ival) {
  static_assert(sizeof(ival) <= sizeof(unsigned long),
                "Unsupported unsigned long long size");
  return PyLong_FromUnsignedLong(ival);
}

PY_EXPORT PyObject* PyLong_FromSize_t(size_t ival) {
  static_assert(sizeof(ival) <= sizeof(unsigned long),
                "Unsupported size_t size");
  return PyLong_FromUnsignedLong(ival);
}

// Attempt to convert the given PyObject to T. When overflow != nullptr,
// *overflow will be set to -1, 1, or 0 to indicate underflow, overflow, or
// neither, respectively. When under/overflow occurs, -1 is returned; otherwise,
// the value is returned.
//
// When overflow == nullptr, an exception will be raised and -1 is returned if
// the value doesn't fit in T.
template <typename T>
static T asInt(PyObject* pylong, const char* type_name, int* overflow) {
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);

  if (pylong == nullptr) {
    thread->raiseSystemErrorWithCStr("bad argument to internal function");
    return -1;
  }

  Handle<Object> longobj(&scope, ApiHandle::fromPyObject(pylong)->asObject());
  if (!longobj->isInt()) {
    // TODO(T29753730): Handle calling __int__ on pylong when appropriate.
    return -1;
  }

  auto const result = Int::cast(*longobj)->asInt<T>();
  if (result.error == CastError::None) {
    if (overflow) *overflow = 0;
    return result.value;
  }

  if (overflow) {
    *overflow = (result.error == CastError::Underflow) ? -1 : 1;
  } else if (result.error == CastError::Underflow &&
             std::is_unsigned<T>::value) {
    thread->raiseOverflowErrorWithCStr(
        "can't convert negative value to unsigned");
  } else {
    thread->raiseOverflowError(thread->runtime()->newStrFromFormat(
        "Python int too big to convert to C %s", type_name));
  }
  return -1;
}

// Converting to signed ints.

PY_EXPORT long PyLong_AsLong(PyObject* pylong) {
  return asInt<long>(pylong, "long", nullptr);
}

PY_EXPORT long long PyLong_AsLongLong(PyObject* val) {
  return asInt<long long>(val, "long long", nullptr);
}

PY_EXPORT Py_ssize_t PyLong_AsSsize_t(PyObject* val) {
  return asInt<Py_ssize_t>(val, "ssize_t", nullptr);
}

// Converting to unsigned ints.

PY_EXPORT unsigned long PyLong_AsUnsignedLong(PyObject* val) {
  return asInt<unsigned long>(val, "unsigned long", nullptr);
}

PY_EXPORT unsigned long long PyLong_AsUnsignedLongLong(PyObject* val) {
  return asInt<unsigned long long>(val, "unsigned long long", nullptr);
}

PY_EXPORT size_t PyLong_AsSize_t(PyObject* val) {
  return asInt<size_t>(val, "size_t", nullptr);
}

PY_EXPORT long PyLong_AsLongAndOverflow(PyObject* pylong, int* overflow) {
  return asInt<long>(pylong, "", overflow);
}

PY_EXPORT long long PyLong_AsLongLongAndOverflow(PyObject* pylong,
                                                 int* overflow) {
  return asInt<long long>(pylong, "", overflow);
}

PY_EXPORT PyObject* PyLong_FromDouble(double /* l */) {
  UNIMPLEMENTED("PyLong_FromDouble");
}

PY_EXPORT PyObject* PyLong_FromString(const char* /* r */, char** /* pend */,
                                      int /* e */) {
  UNIMPLEMENTED("PyLong_FromString");
}

PY_EXPORT double PyLong_AsDouble(PyObject* /* v */) {
  UNIMPLEMENTED("PyLong_AsDouble");
}

PY_EXPORT unsigned long long PyLong_AsUnsignedLongLongMask(PyObject* /* p */) {
  UNIMPLEMENTED("PyLong_AsUnsignedLongLongMask");
}

PY_EXPORT unsigned long PyLong_AsUnsignedLongMask(PyObject* /* p */) {
  UNIMPLEMENTED("PyLong_AsUnsignedLongMask");
}

PY_EXPORT void* PyLong_AsVoidPtr(PyObject* /* v */) {
  UNIMPLEMENTED("PyLong_AsVoidPtr");
}

PY_EXPORT PyObject* PyLong_FromVoidPtr(void* /* p */) {
  UNIMPLEMENTED("PyLong_FromVoidPtr");
}

PY_EXPORT PyObject* PyLong_GetInfo() { UNIMPLEMENTED("PyLong_GetInfo"); }

}  // namespace python

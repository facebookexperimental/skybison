#include "capi-handles.h"
#include "runtime.h"

namespace py {

PY_EXPORT int PyCFunction_ClearFreeList() { return 0; }

PY_EXPORT PyObject* PyCFunction_NewEx(PyMethodDef* /* l */, PyObject* /* f */,
                                      PyObject* /* e */) {
  UNIMPLEMENTED("PyCFunction_NewEx");
}

PY_EXPORT int PyCFunction_GetFlags(PyObject* /* p */) {
  UNIMPLEMENTED("PyCFunction_GetFlags");
}

PY_EXPORT PyCFunction PyCFunction_GetFunction(PyObject* /* p */) {
  UNIMPLEMENTED("PyCFunction_GetFunction");
}

PY_EXPORT PyObject* PyCFunction_GetSelf(PyObject* /* p */) {
  UNIMPLEMENTED("PyCFunction_GetSelf");
}

PY_EXPORT PyObject* PyCFunction_Call(PyObject* /* c */, PyObject* /* s */,
                                     PyObject* /* s */) {
  UNIMPLEMENTED("PyCFunction_Call");
}

}  // namespace py

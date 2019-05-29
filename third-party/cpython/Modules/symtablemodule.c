#include "Python.h"

#include "code.h"
#include "Python-ast.h"
#include "symtable.h"

static PyObject *
symtable_symtable(PyObject *self, PyObject *args)
{
    struct symtable *st;
    PyObject *t;

    char *str;
    PyObject *filename;
    char *startstr;
    int start;

    if (!PyArg_ParseTuple(args, "sO&s:symtable",
                          &str, PyUnicode_FSDecoder, &filename, &startstr))
        return NULL;
    if (strcmp(startstr, "exec") == 0)
        start = Py_file_input;
    else if (strcmp(startstr, "eval") == 0)
        start = Py_eval_input;
    else if (strcmp(startstr, "single") == 0)
        start = Py_single_input;
    else {
        PyErr_SetString(PyExc_ValueError,
           "symtable() arg 3 must be 'exec' or 'eval' or 'single'");
        Py_DECREF(filename);
        return NULL;
    }
    st = Py_SymtableStringObject(str, filename, start);
    Py_DECREF(filename);
    if (st == NULL)
        return NULL;
    t = (PyObject *)st->st_top;
    Py_INCREF(t);
    PyMem_Free((void *)st->st_future);
    PySymtable_Free(st);
    return t;
}

static PyMethodDef symtable_methods[] = {
    {"symtable",        symtable_symtable,      METH_VARARGS,
     PyDoc_STR("Return symbol and scope dictionaries"
               " used internally by compiler.")},
    {NULL,              NULL}           /* sentinel */
};

static struct PyModuleDef symtablemodule = {
    PyModuleDef_HEAD_INIT,
    "_symtable",
    NULL,
    -1,
    symtable_methods,
    NULL,
    NULL,
    NULL,
    NULL
};

PyMODINIT_FUNC
PyInit__symtable(void)
{
    PyObject *m;

    m = PyModule_Create(&symtablemodule);
    if (m == NULL)
        return NULL;
    PyModule_AddIntMacro(m, USE);
    PyModule_AddIntMacro(m, DEF_GLOBAL);
    PyModule_AddIntMacro(m, DEF_LOCAL);
    PyModule_AddIntMacro(m, DEF_PARAM);
    PyModule_AddIntMacro(m, DEF_FREE);
    PyModule_AddIntMacro(m, DEF_FREE_CLASS);
    PyModule_AddIntMacro(m, DEF_IMPORT);
    PyModule_AddIntMacro(m, DEF_BOUND);
    PyModule_AddIntMacro(m, DEF_ANNOT);

    PyModule_AddIntConstant(m, "TYPE_FUNCTION", FunctionBlock);
    PyModule_AddIntConstant(m, "TYPE_CLASS", ClassBlock);
    PyModule_AddIntConstant(m, "TYPE_MODULE", ModuleBlock);

    PyModule_AddIntMacro(m, LOCAL);
    PyModule_AddIntMacro(m, GLOBAL_EXPLICIT);
    PyModule_AddIntMacro(m, GLOBAL_IMPLICIT);
    PyModule_AddIntMacro(m, FREE);
    PyModule_AddIntMacro(m, CELL);

    PyModule_AddIntConstant(m, "SCOPE_OFF", SCOPE_OFFSET);
    PyModule_AddIntMacro(m, SCOPE_MASK);

    if (PyErr_Occurred()) {
        Py_DECREF(m);
        m = 0;
    }
    return m;
}

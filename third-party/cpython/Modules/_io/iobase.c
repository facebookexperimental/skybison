/*
    An implementation of the I/O abstract base classes hierarchy
    as defined by PEP 3116 - "New I/O"

    Classes defined here: IOBase, RawIOBase.

    Written by Amaury Forgeot d'Arc and Antoine Pitrou
*/


#define PY_SSIZE_T_CLEAN
#include "Python.h"
#include "structmember.h"
#include "_iomodule.h"

/*[clinic input]
module _io
class _io._IOBase "PyObject *" "(PyTypeObject *)IO_MOD_STATE_GLOBAL->PyIOBase_Type"
class _io._RawIOBase "PyObject *" "(PyTypeObject *)IO_MOD_STATE_GLOBAL->PyRawIOBase_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=4c8627df95d9d0b1]*/

/*[python input]
class io_ssize_t_converter(CConverter):
    type = 'Py_ssize_t'
    converter = '_PyIO_ConvertSsize_t'
[python start generated code]*/
/*[python end generated code: output=da39a3ee5e6b4b0d input=d0a811d3cbfd1b33]*/

/*
 * IOBase class, an abstract class
 */

PyDoc_STRVAR(iobase_doc,
    "The abstract base class for all I/O classes, acting on streams of\n"
    "bytes. There is no public constructor.\n"
    "\n"
    "This class provides dummy implementations for many methods that\n"
    "derived classes can override selectively; the default implementations\n"
    "represent a file that cannot be read, written or seeked.\n"
    "\n"
    "Even though IOBase does not declare read, readinto, or write because\n"
    "their signatures will vary, implementations and clients should\n"
    "consider those methods part of the interface. Also, implementations\n"
    "may raise UnsupportedOperation when operations they do not support are\n"
    "called.\n"
    "\n"
    "The basic type used for binary data read from or written to a file is\n"
    "bytes. Other bytes-like objects are accepted as method arguments too.\n"
    "In some cases (such as readinto), a writable object is required. Text\n"
    "I/O classes work with str data.\n"
    "\n"
    "Note that calling any method (except additional calls to close(),\n"
    "which are ignored) on a closed stream should raise a ValueError.\n"
    "\n"
    "IOBase (and its subclasses) support the iterator protocol, meaning\n"
    "that an IOBase object can be iterated over yielding the lines in a\n"
    "stream.\n"
    "\n"
    "IOBase also supports the :keyword:`with` statement. In this example,\n"
    "fp is closed after the suite of the with statement is complete:\n"
    "\n"
    "with open('spam.txt', 'r') as fp:\n"
    "    fp.write('Spam and eggs!')\n");

/* Use this macro whenever you want to check the internal `closed` status
   of the IOBase object rather than the virtual `closed` attribute as returned
   by whatever subclass. */

#define IS_CLOSED(self) \
    PyObject_HasAttr(self, IO_MOD_STATE_GLOBAL->__IOBase_closed)

/* Internal methods */
static PyObject *
iobase_unsupported(const char *message)
{
    _PyIO_State *state = IO_STATE();
    if (state != NULL)
        PyErr_SetString(state->unsupported_operation, message);
    return NULL;
}

/* Positioning */

PyDoc_STRVAR(iobase_seek_doc,
    "Change stream position.\n"
    "\n"
    "Change the stream position to the given byte offset. The offset is\n"
    "interpreted relative to the position indicated by whence.  Values\n"
    "for whence are:\n"
    "\n"
    "* 0 -- start of stream (the default); offset should be zero or positive\n"
    "* 1 -- current stream position; offset may be negative\n"
    "* 2 -- end of stream; offset is usually negative\n"
    "\n"
    "Return the new absolute position.");

static PyObject *
iobase_seek(PyObject *self, PyObject *args)
{
    return iobase_unsupported("seek");
}

/*[clinic input]
_io._IOBase.tell

Return current stream position.
[clinic start generated code]*/

static PyObject *
_io__IOBase_tell_impl(PyObject *self)
/*[clinic end generated code: output=89a1c0807935abe2 input=04e615fec128801f]*/
{
    const char* seek_str = PyUnicode_AsUTF8(IO_MOD_STATE_GLOBAL->seek);
    return PyObject_CallMethod(self, seek_str, "ii", 0, 1);
}

PyDoc_STRVAR(iobase_truncate_doc,
    "Truncate file to size bytes.\n"
    "\n"
    "File pointer is left unchanged.  Size defaults to the current IO\n"
    "position as reported by tell().  Returns the new size.");

static PyObject *
iobase_truncate(PyObject *self, PyObject *args)
{
    return iobase_unsupported("truncate");
}

/* Flush and close methods */

/*[clinic input]
_io._IOBase.flush

Flush write buffers, if applicable.

This is not implemented for read-only and non-blocking streams.
[clinic start generated code]*/

static PyObject *
_io__IOBase_flush_impl(PyObject *self)
/*[clinic end generated code: output=7cef4b4d54656a3b input=773be121abe270aa]*/
{
    /* XXX Should this return the number of bytes written??? */
    if (IS_CLOSED(self)) {
        PyErr_SetString(PyExc_ValueError, "I/O operation on closed file.");
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject *
iobase_dunder_dict(iobase *self, void *context)
{
    if (self->dict == NULL) {
        self->dict = PyDict_New();
    }
    Py_INCREF(self->dict);
    return self->dict;
}

static PyObject *
iobase_getattro(iobase *self, PyObject *name)
{
    if (self->dict == NULL) {
        self->dict = PyDict_New();
    }
    return _PyObject_GenericGetAttrWithDict((PyObject *)self, name, self->dict);
}

static int
iobase_setattro(iobase *self, PyObject *name, PyObject *value)
{
    if (self->dict == NULL) {
        self->dict = PyDict_New();
    }
    return _PyObject_GenericSetAttrWithDict((PyObject *)self, name, value, self->dict);
}

static int
iobase_closed(PyObject *self)
{
    PyObject *res;
    int closed;
    /* This gets the derived attribute, which is *not* __IOBase_closed
       in most cases! */
    res = PyObject_GetAttr(self, IO_MOD_STATE_GLOBAL->closed);
    if (res == NULL)
        return 0;
    closed = PyObject_IsTrue(res);
    Py_DECREF(res);
    return closed;
}

static PyObject *
iobase_closed_get(PyObject *self, void *context)
{
    return PyBool_FromLong(IS_CLOSED(self));
}

PyObject *
_PyIOBase_check_closed(PyObject *self, PyObject *args)
{
    if (iobase_closed(self)) {
        PyErr_SetString(PyExc_ValueError, "I/O operation on closed file.");
        return NULL;
    }
    if (args == Py_True)
        return Py_None;
    else
        Py_RETURN_NONE;
}

/* XXX: IOBase thinks it has to maintain its own internal state in
   `__IOBase_closed` and call flush() by itself, but it is redundant with
   whatever behaviour a non-trivial derived class will implement. */

/*[clinic input]
_io._IOBase.close

Flush and close the IO object.

This method has no effect if the file is already closed.
[clinic start generated code]*/

static PyObject *
_io__IOBase_close_impl(PyObject *self)
/*[clinic end generated code: output=63c6a6f57d783d6d input=f4494d5c31dbc6b7]*/
{
    PyObject *res, *exc, *val, *tb;
    int rc;

    if (IS_CLOSED(self))
        Py_RETURN_NONE;

    res = PyObject_CallMethodObjArgs(self, IO_MOD_STATE_GLOBAL->flush, NULL);

    PyErr_Fetch(&exc, &val, &tb);
    rc = PyObject_SetAttr(self, IO_MOD_STATE_GLOBAL->__IOBase_closed, Py_True);
    _PyErr_ChainExceptions(exc, val, tb);
    if (rc < 0) {
        Py_CLEAR(res);
    }

    if (res == NULL)
        return NULL;

    Py_DECREF(res);
    Py_RETURN_NONE;
}

/* Finalization and garbage collection support */

static void
iobase_finalize(PyObject *self)
{
    PyObject *res;
    PyObject *error_type, *error_value, *error_traceback;
    int closed;

    /* Save the current exception, if any. */
    PyErr_Fetch(&error_type, &error_value, &error_traceback);

    /* If `closed` doesn't exist or can't be evaluated as bool, then the
       object is probably in an unusable state, so ignore. */
    res = PyObject_GetAttr(self, IO_MOD_STATE_GLOBAL->closed);
    if (res == NULL) {
        PyErr_Clear();
        closed = -1;
    }
    else {
        closed = PyObject_IsTrue(res);
        Py_DECREF(res);
        if (closed == -1)
            PyErr_Clear();
    }
    if (closed == 0) {
        /* Signal close() that it was called as part of the object
           finalization process. */
        if (PyObject_SetAttr(self, IO_MOD_STATE_GLOBAL->_finalizing, Py_True))
            PyErr_Clear();
        res = PyObject_CallMethodObjArgs((PyObject *) self, IO_MOD_STATE_GLOBAL->close,
                                          NULL);
        /* Silencing I/O errors is bad, but printing spurious tracebacks is
           equally as bad, and potentially more frequent (because of
           shutdown issues). */
        if (res == NULL)
            PyErr_Clear();
        else
            Py_DECREF(res);
    }

    /* Restore the saved exception. */
    PyErr_Restore(error_type, error_value, error_traceback);
}

int
_PyIOBase_finalize(PyObject *self)
{
    int is_zombie;

    /* If _PyIOBase_finalize() is called from a destructor, we need to
       resurrect the object as calling close() can invoke arbitrary code. */
    is_zombie = (Py_REFCNT(self) == 0);
    if (is_zombie)
        return PyObject_CallFinalizerFromDealloc(self);
    else {
        PyObject_CallFinalizer(self);
        return 0;
    }
}

static int
iobase_traverse(iobase *self, visitproc visit, void *arg)
{
    Py_VISIT(self->dict);
    return 0;
}

static int
iobase_clear(iobase *self)
{
    Py_CLEAR(self->dict);
    return 0;
}

/* Destructor */

static void
iobase_dealloc(iobase *self)
{
    /* NOTE: since IOBaseObject has its own dict, Python-defined attributes
       are still available here for close() to use.
       However, if the derived class declares a __slots__, those slots are
       already gone.
    */
    PyTypeObject *tp = Py_TYPE(self);
    if (_PyIOBase_finalize((PyObject *) self) < 0) {
        return;
    }
    _PyObject_GC_UNTRACK(self);
    if (self->weakreflist != NULL)
        PyObject_ClearWeakRefs((PyObject *) self);
    Py_CLEAR(self->dict);
    tp->tp_free((PyObject *) self);
    Py_DECREF(tp);
}

/* Inquiry methods */

/*[clinic input]
_io._IOBase.seekable

Return whether object supports random access.

If False, seek(), tell() and truncate() will raise OSError.
This method may need to do a test seek().
[clinic start generated code]*/

static PyObject *
_io__IOBase_seekable_impl(PyObject *self)
/*[clinic end generated code: output=4c24c67f5f32a43d input=b976622f7fdf3063]*/
{
    Py_RETURN_FALSE;
}

PyObject *
_PyIOBase_check_seekable(PyObject *self, PyObject *args)
{
    PyObject *res  = PyObject_CallMethodObjArgs(self, IO_MOD_STATE_GLOBAL->seekable, NULL);
    if (res == NULL)
        return NULL;
    if (res != Py_True) {
        Py_CLEAR(res);
        iobase_unsupported("File or stream is not seekable.");
        return NULL;
    }
    if (args == Py_True) {
        Py_DECREF(res);
    }
    return res;
}

/*[clinic input]
_io._IOBase.readable

Return whether object was opened for reading.

If False, read() will raise OSError.
[clinic start generated code]*/

static PyObject *
_io__IOBase_readable_impl(PyObject *self)
/*[clinic end generated code: output=e48089250686388b input=285b3b866a0ec35f]*/
{
    Py_RETURN_FALSE;
}

/* May be called with any object */
PyObject *
_PyIOBase_check_readable(PyObject *self, PyObject *args)
{
    PyObject *res  = PyObject_CallMethodObjArgs(self, IO_MOD_STATE_GLOBAL->readable, NULL);
    if (res == NULL)
        return NULL;
    if (res != Py_True) {
        Py_CLEAR(res);
        iobase_unsupported("File or stream is not readable.");
        return NULL;
    }
    if (args == Py_True) {
        Py_DECREF(res);
    }
    return res;
}

/*[clinic input]
_io._IOBase.writable

Return whether object was opened for writing.

If False, write() will raise OSError.
[clinic start generated code]*/

static PyObject *
_io__IOBase_writable_impl(PyObject *self)
/*[clinic end generated code: output=406001d0985be14f input=9dcac18a013a05b5]*/
{
    Py_RETURN_FALSE;
}

/* May be called with any object */
PyObject *
_PyIOBase_check_writable(PyObject *self, PyObject *args)
{
    PyObject *res  = PyObject_CallMethodObjArgs(self, IO_MOD_STATE_GLOBAL->writable, NULL);
    if (res == NULL)
        return NULL;
    if (res != Py_True) {
        Py_CLEAR(res);
        iobase_unsupported("File or stream is not writable.");
        return NULL;
    }
    if (args == Py_True) {
        Py_DECREF(res);
    }
    return res;
}

/* Context manager */

static PyObject *
iobase_enter(PyObject *self, PyObject *args)
{
    if (_PyIOBase_check_closed(self, Py_True) == NULL)
        return NULL;

    Py_INCREF(self);
    return self;
}

static PyObject *
iobase_exit(PyObject *self, PyObject *args)
{
    return PyObject_CallMethodObjArgs(self, IO_MOD_STATE_GLOBAL->close, NULL);
}

/* Lower-level APIs */

/* XXX Should these be present even if unimplemented? */

/*[clinic input]
_io._IOBase.fileno

Returns underlying file descriptor if one exists.

OSError is raised if the IO object does not use a file descriptor.
[clinic start generated code]*/

static PyObject *
_io__IOBase_fileno_impl(PyObject *self)
/*[clinic end generated code: output=7cc0973f0f5f3b73 input=4e37028947dc1cc8]*/
{
    return iobase_unsupported("fileno");
}

/*[clinic input]
_io._IOBase.isatty

Return whether this is an 'interactive' stream.

Return False if it can't be determined.
[clinic start generated code]*/

static PyObject *
_io__IOBase_isatty_impl(PyObject *self)
/*[clinic end generated code: output=60cab77cede41cdd input=9ef76530d368458b]*/
{
    if (_PyIOBase_check_closed(self, Py_True) == NULL)
        return NULL;
    Py_RETURN_FALSE;
}

/* Readline(s) and writelines */

/*[clinic input]
_io._IOBase.readline
    size as limit: io_ssize_t = -1
    /

Read and return a line from the stream.

If size is specified, at most size bytes will be read.

The line terminator is always b'\n' for binary files; for text
files, the newlines argument to open can be used to select the line
terminator(s) recognized.
[clinic start generated code]*/

static PyObject *
_io__IOBase_readline_impl(PyObject *self, Py_ssize_t limit)
/*[clinic end generated code: output=4479f79b58187840 input=df4cc8884f553cab]*/
{
    /* For backwards compatibility, a (slowish) readline(). */

    int has_peek = 0;
    PyObject *buffer, *result;
    Py_ssize_t old_size = -1;
    _Py_IDENTIFIER(peek);

    if (PyObject_HasAttr(self, IO_MOD_STATE_GLOBAL->peek))
        has_peek = 1;

    buffer = PyByteArray_FromStringAndSize(NULL, 0);
    if (buffer == NULL)
        return NULL;

    while (limit < 0 || Py_SIZE(buffer) < limit) {
        Py_ssize_t nreadahead = 1;
        PyObject *b;

        if (has_peek) {
            PyObject *readahead = _PyObject_CallMethodId(self, &PyId_peek, "i", 1);
            if (readahead == NULL) {
                /* NOTE: PyErr_SetFromErrno() calls PyErr_CheckSignals()
                   when EINTR occurs so we needn't do it ourselves. */
                if (_PyIO_trap_eintr()) {
                    continue;
                }
                goto fail;
            }
            if (!PyBytes_Check(readahead)) {
                PyErr_Format(PyExc_IOError,
                             "peek() should have returned a bytes object, "
                             "not '%.200s'", _PyType_Name(Py_TYPE(readahead)));
                Py_DECREF(readahead);
                goto fail;
            }
            if (PyBytes_GET_SIZE(readahead) > 0) {
                Py_ssize_t n = 0;
                const char *buf = PyBytes_AS_STRING(readahead);
                if (limit >= 0) {
                    do {
                        if (n >= PyBytes_GET_SIZE(readahead) || n >= limit)
                            break;
                        if (buf[n++] == '\n')
                            break;
                    } while (1);
                }
                else {
                    do {
                        if (n >= PyBytes_GET_SIZE(readahead))
                            break;
                        if (buf[n++] == '\n')
                            break;
                    } while (1);
                }
                nreadahead = n;
            }
            Py_DECREF(readahead);
        }

        const char *read_str = PyUnicode_AsUTF8(IO_MOD_STATE_GLOBAL->read);
        b = PyObject_CallMethod(self, read_str, "n", nreadahead);
        if (b == NULL) {
            /* NOTE: PyErr_SetFromErrno() calls PyErr_CheckSignals()
               when EINTR occurs so we needn't do it ourselves. */
            if (_PyIO_trap_eintr()) {
                continue;
            }
            goto fail;
        }
        if (!PyBytes_Check(b)) {
            PyErr_Format(PyExc_IOError,
                         "read() should have returned a bytes object, "
                         "not '%.200s'", _PyType_Name(Py_TYPE(b)));
            Py_DECREF(b);
            goto fail;
        }
        if (PyBytes_GET_SIZE(b) == 0) {
            Py_DECREF(b);
            break;
        }

        old_size = PyByteArray_GET_SIZE(buffer);
        if (PyByteArray_Resize(buffer, old_size + PyBytes_GET_SIZE(b)) < 0) {
            Py_DECREF(b);
            goto fail;
        }
        memcpy(PyByteArray_AS_STRING(buffer) + old_size,
               PyBytes_AS_STRING(b), PyBytes_GET_SIZE(b));

        Py_DECREF(b);

        if (PyByteArray_AS_STRING(buffer)[PyByteArray_GET_SIZE(buffer) - 1] == '\n')
            break;
    }

    result = PyBytes_FromStringAndSize(PyByteArray_AS_STRING(buffer),
                                       PyByteArray_GET_SIZE(buffer));
    Py_DECREF(buffer);
    return result;
  fail:
    Py_DECREF(buffer);
    return NULL;
}

static PyObject *
iobase_iter(PyObject *self)
{
    if (_PyIOBase_check_closed(self, Py_True) == NULL)
        return NULL;

    Py_INCREF(self);
    return self;
}

static PyObject *
iobase_iternext(PyObject *self)
{
    PyObject *line = PyObject_CallMethodObjArgs(self, IO_MOD_STATE_GLOBAL->readline, NULL);

    if (line == NULL)
        return NULL;

    if (PyObject_Size(line) <= 0) {
        /* Error or empty */
        Py_DECREF(line);
        return NULL;
    }

    return line;
}

/*[clinic input]
_io._IOBase.readlines
    hint: io_ssize_t = -1
    /

Return a list of lines from the stream.

hint can be specified to control the number of lines read: no more
lines will be read if the total size (in bytes/characters) of all
lines so far exceeds hint.
[clinic start generated code]*/

static PyObject *
_io__IOBase_readlines_impl(PyObject *self, Py_ssize_t hint)
/*[clinic end generated code: output=2f50421677fa3dea input=1961c4a95e96e661]*/
{
    Py_ssize_t length = 0;
    PyObject *result, *it = NULL;

    result = PyList_New(0);
    if (result == NULL)
        return NULL;

    if (hint <= 0) {
        /* XXX special-casing this made sense in the Python version in order
           to remove the bytecode interpretation overhead, but it could
           probably be removed here. */
        PyObject *ret = PyObject_CallMethodObjArgs(result, IO_MOD_STATE_GLOBAL->extend,
                                                      self, NULL);

        if (ret == NULL) {
            goto error;
        }
        Py_DECREF(ret);
        return result;
    }

    it = PyObject_GetIter(self);
    if (it == NULL) {
        goto error;
    }

    while (1) {
        Py_ssize_t line_length;
        PyObject *line = PyIter_Next(it);
        if (line == NULL) {
            if (PyErr_Occurred()) {
                goto error;
            }
            else
                break; /* StopIteration raised */
        }

        if (PyList_Append(result, line) < 0) {
            Py_DECREF(line);
            goto error;
        }
        line_length = PyObject_Size(line);
        Py_DECREF(line);
        if (line_length < 0) {
            goto error;
        }
        if (line_length > hint - length)
            break;
        length += line_length;
    }

    Py_DECREF(it);
    return result;

 error:
    Py_XDECREF(it);
    Py_DECREF(result);
    return NULL;
}

/*[clinic input]
_io._IOBase.writelines
    lines: object
    /
[clinic start generated code]*/

static PyObject *
_io__IOBase_writelines(PyObject *self, PyObject *lines)
/*[clinic end generated code: output=976eb0a9b60a6628 input=432e729a8450b3cb]*/
{
    PyObject *iter, *res;

    if (_PyIOBase_check_closed(self, Py_True) == NULL)
        return NULL;

    iter = PyObject_GetIter(lines);
    if (iter == NULL)
        return NULL;

    while (1) {
        PyObject *line = PyIter_Next(iter);
        if (line == NULL) {
            if (PyErr_Occurred()) {
                Py_DECREF(iter);
                return NULL;
            }
            else
                break; /* Stop Iteration */
        }

        res = NULL;
        do {
            res = PyObject_CallMethodObjArgs(self, IO_MOD_STATE_GLOBAL->write, line, NULL);
        } while (res == NULL && _PyIO_trap_eintr());
        Py_DECREF(line);
        if (res == NULL) {
            Py_DECREF(iter);
            return NULL;
        }
        Py_DECREF(res);
    }
    Py_DECREF(iter);
    Py_RETURN_NONE;
}

#include "clinic/iobase.c.h"

static PyMemberDef iobase_members[] = {
    {"__weaklistoffset__", T_NONE, offsetof(iobase, weakreflist), READONLY},
    {NULL}
};

static PyMethodDef iobase_methods[] = {
    {"seek", iobase_seek, METH_VARARGS, iobase_seek_doc},
    _IO__IOBASE_TELL_METHODDEF
    {"truncate", iobase_truncate, METH_VARARGS, iobase_truncate_doc},
    _IO__IOBASE_FLUSH_METHODDEF
    _IO__IOBASE_CLOSE_METHODDEF

    _IO__IOBASE_SEEKABLE_METHODDEF
    _IO__IOBASE_READABLE_METHODDEF
    _IO__IOBASE_WRITABLE_METHODDEF

    {"_checkClosed",   _PyIOBase_check_closed, METH_NOARGS},
    {"_checkSeekable", _PyIOBase_check_seekable, METH_NOARGS},
    {"_checkReadable", _PyIOBase_check_readable, METH_NOARGS},
    {"_checkWritable", _PyIOBase_check_writable, METH_NOARGS},

    _IO__IOBASE_FILENO_METHODDEF
    _IO__IOBASE_ISATTY_METHODDEF

    {"__enter__", iobase_enter, METH_NOARGS},
    {"__exit__", iobase_exit, METH_VARARGS},

    _IO__IOBASE_READLINE_METHODDEF
    _IO__IOBASE_READLINES_METHODDEF
    _IO__IOBASE_WRITELINES_METHODDEF

    {NULL, NULL}
};

static PyGetSetDef iobase_getset[] = {
    {"__dict__", (getter)iobase_dunder_dict, NULL, NULL},
    {"closed", (getter)iobase_closed_get, NULL, NULL},
    {NULL}
};


PyType_Slot PyIOBase_Type_slots[] = {
    {Py_tp_dealloc, iobase_dealloc},
    {Py_tp_getattro, iobase_getattro},
    {Py_tp_setattro, iobase_setattro},
    {Py_tp_doc, iobase_doc},
    {Py_tp_traverse, iobase_traverse},
    {Py_tp_clear, iobase_clear},
    {Py_tp_iter, iobase_iter},
    {Py_tp_iternext, iobase_iternext},
    {Py_tp_members, iobase_members},
    {Py_tp_methods, iobase_methods},
    {Py_tp_getset, iobase_getset},
    {Py_tp_new, PyType_GenericNew},
    {Py_tp_finalize, iobase_finalize},
    {0, 0},
};

PyType_Spec PyIOBase_Type_spec = {
    "_io._IOBase",
    sizeof(iobase),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_HAVE_FINALIZE,
    PyIOBase_Type_slots
};

/*
 * RawIOBase class, Inherits from IOBase.
 */
PyDoc_STRVAR(rawiobase_doc,
             "Base class for raw binary I/O.");

/*
 * The read() method is implemented by calling readinto(); derived classes
 * that want to support read() only need to implement readinto() as a
 * primitive operation.  In general, readinto() can be more efficient than
 * read().
 *
 * (It would be tempting to also provide an implementation of readinto() in
 * terms of read(), in case the latter is a more suitable primitive operation,
 * but that would lead to nasty recursion in case a subclass doesn't implement
 * either.)
*/

/*[clinic input]
_io._RawIOBase.read
    size as n: Py_ssize_t = -1
    /
[clinic start generated code]*/

static PyObject *
_io__RawIOBase_read_impl(PyObject *self, Py_ssize_t n)
/*[clinic end generated code: output=6cdeb731e3c9f13c input=b6d0dcf6417d1374]*/
{
    PyObject *b, *res;

    if (n < 0) {
        return PyObject_CallMethodObjArgs(self, IO_MOD_STATE_GLOBAL->readall, NULL);
    }

    /* TODO: allocate a bytes object directly instead and manually construct
       a writable memoryview pointing to it. */
    b = PyByteArray_FromStringAndSize(NULL, n);
    if (b == NULL)
        return NULL;

    res = PyObject_CallMethodObjArgs(self, IO_MOD_STATE_GLOBAL->readinto, b, NULL);
    if (res == NULL || res == Py_None) {
        Py_DECREF(b);
        return res;
    }

    n = PyNumber_AsSsize_t(res, PyExc_ValueError);
    Py_DECREF(res);
    if (n == -1 && PyErr_Occurred()) {
        Py_DECREF(b);
        return NULL;
    }

    res = PyBytes_FromStringAndSize(PyByteArray_AsString(b), n);
    Py_DECREF(b);
    return res;
}


/*[clinic input]
_io._RawIOBase.readall

Read until EOF, using multiple read() call.
[clinic start generated code]*/

static PyObject *
_io__RawIOBase_readall_impl(PyObject *self)
/*[clinic end generated code: output=1987b9ce929425a0 input=688874141213622a]*/
{
    int r;
    PyObject *chunks = PyList_New(0);
    PyObject *result;

    if (chunks == NULL)
        return NULL;

    while (1) {
        const char *read_str = PyUnicode_AsUTF8(IO_MOD_STATE_GLOBAL->read);
        PyObject *data = PyObject_CallMethod(self, read_str,
                                                    "i", DEFAULT_BUFFER_SIZE);
        if (!data) {
            /* NOTE: PyErr_SetFromErrno() calls PyErr_CheckSignals()
               when EINTR occurs so we needn't do it ourselves. */
            if (_PyIO_trap_eintr()) {
                continue;
            }
            Py_DECREF(chunks);
            return NULL;
        }
        if (data == Py_None) {
            if (PyList_GET_SIZE(chunks) == 0) {
                Py_DECREF(chunks);
                return data;
            }
            Py_DECREF(data);
            break;
        }
        if (!PyBytes_Check(data)) {
            Py_DECREF(chunks);
            Py_DECREF(data);
            PyErr_SetString(PyExc_TypeError, "read() should return bytes");
            return NULL;
        }
        if (PyBytes_GET_SIZE(data) == 0) {
            /* EOF */
            Py_DECREF(data);
            break;
        }
        r = PyList_Append(chunks, data);
        Py_DECREF(data);
        if (r < 0) {
            Py_DECREF(chunks);
            return NULL;
        }
    }
    result = _PyBytes_Join(IO_MOD_STATE_GLOBAL->empty_bytes, chunks);
    Py_DECREF(chunks);
    return result;
}

static PyObject *
rawiobase_readinto(PyObject *self, PyObject *args)
{
    PyErr_SetNone(PyExc_NotImplementedError);
    return NULL;
}

static PyObject *
rawiobase_write(PyObject *self, PyObject *args)
{
    PyErr_SetNone(PyExc_NotImplementedError);
    return NULL;
}

static PyMethodDef rawiobase_methods[] = {
    _IO__RAWIOBASE_READ_METHODDEF
    _IO__RAWIOBASE_READALL_METHODDEF
    {"readinto", rawiobase_readinto, METH_VARARGS},
    {"write", rawiobase_write, METH_VARARGS},
    {NULL, NULL}
};

PyType_Slot PyRawIOBase_Type_slots[] = {
    {Py_tp_doc, rawiobase_doc},
    {Py_tp_methods, rawiobase_methods},
    {0, 0},
};

PyType_Spec PyRawIOBase_Type_spec = {
    "_io._RawIOBase",
    0,
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_FINALIZE,
    PyRawIOBase_Type_slots
};

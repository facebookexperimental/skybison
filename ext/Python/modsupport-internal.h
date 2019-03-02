#pragma once

// This exposes some modsupport code to abstract.cpp; it is not cpython API.

#include <cstdarg>

#include "cpython-types.h"

namespace python {

const int kFlagSizeT = 1;

PyObject* makeValueFromFormat(const char** format, std::va_list* va, int flags);

}  // namespace python

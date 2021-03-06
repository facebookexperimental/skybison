// Copyright (c) Facebook, Inc. and its affiliates. (http://www.facebook.com)
#include <strings.h>

#include "cpython-func.h"

#include "api-handle.h"

namespace py {

PY_EXPORT int PyOS_stricmp(const char* s1, const char* s2) {
  return ::strcasecmp(s1, s2);
}

PY_EXPORT int PyOS_strnicmp(const char* s1, const char* s2, Py_ssize_t size) {
  return ::strncasecmp(s1, s2, size);
}

}  // namespace py

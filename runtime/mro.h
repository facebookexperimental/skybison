#pragma once

#include "handles.h"

namespace python {

class RawObject;
class RawType;
class RawTuple;
class Thread;

// Given the list of immediate parents, compute the complete MRO
// for the type's ancestors via C3 linearization
RawObject computeMro(Thread* thread, const Type& type, const Tuple& parents);

}  // namespace python

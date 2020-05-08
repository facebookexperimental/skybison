#pragma once

#include "frame.h"
#include "globals.h"
#include "modules.h"
#include "objects.h"
#include "runtime.h"

namespace py {

class ArrayModule {
 public:
  static void initialize(Thread* thread, const Module& module);

 private:
  static const BuiltinType kBuiltinTypes[];

  DISALLOW_IMPLICIT_CONSTRUCTORS(ArrayModule);
};

void initializeArrayType(Thread* thread);

}  // namespace py

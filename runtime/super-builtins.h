#pragma once

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace py {

RawObject superGetAttribute(Thread* thread, const Super& super,
                            const Object& name);

class SuperBuiltins
    : public Builtins<SuperBuiltins, ID(super), LayoutId::kSuper> {
 public:
  static const BuiltinAttribute kAttributes[];
};

}  // namespace py

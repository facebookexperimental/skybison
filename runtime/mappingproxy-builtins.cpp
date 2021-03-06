// Copyright (c) Facebook, Inc. and its affiliates. (http://www.facebook.com)
#include "mappingproxy-builtins.h"

#include "builtins.h"
#include "type-builtins.h"

namespace py {

static const BuiltinAttribute kMappingProxyAttributes[] = {
    {ID(_mappingproxy__mapping), RawMappingProxy::kMappingOffset,
     AttributeFlags::kHidden},
};

void initializeMappingProxyType(Thread* thread) {
  addBuiltinType(thread, ID(mappingproxy), LayoutId::kMappingProxy,
                 /*superclass_id=*/LayoutId::kObject, kMappingProxyAttributes,
                 MappingProxy::kSize, /*basetype=*/false);
}

}  // namespace py

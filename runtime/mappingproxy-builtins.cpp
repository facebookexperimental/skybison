#include "mappingproxy-builtins.h"

#include "builtins.h"

namespace py {

const BuiltinAttribute MappingProxyBuiltins::kAttributes[] = {
    {ID(_mappingproxy__mapping), RawMappingProxy::kMappingOffset,
     AttributeFlags::kHidden},
    {SymbolId::kSentinelId, -1},
};

}  // namespace py

// Copyright (c) Facebook, Inc. and its affiliates. (http://www.facebook.com)
#include "frame-proxy-builtins.h"

#include "builtins.h"
#include "type-builtins.h"

namespace py {

static const BuiltinAttribute kFrameProxyAttributes[] = {
    {ID(f_back), RawFrameProxy::kBackOffset, AttributeFlags::kReadOnly},
    {ID(_function), RawFrameProxy::kFunctionOffset, AttributeFlags::kReadOnly},
    {ID(f_lasti), RawFrameProxy::kLastiOffset, AttributeFlags::kReadOnly},
    {ID(f_locals), RawFrameProxy::kLocalsOffset, AttributeFlags::kReadOnly},
};

void initializeFrameProxyType(Thread* thread) {
  addBuiltinType(thread, ID(frame), LayoutId::kFrameProxy,
                 /*superclass_id=*/LayoutId::kObject, kFrameProxyAttributes,
                 FrameProxy::kSize, /*basetype=*/false);
}

}  // namespace py

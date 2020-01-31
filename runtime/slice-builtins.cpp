#include "slice-builtins.h"

#include "frame.h"
#include "int-builtins.h"
#include "objects.h"
#include "runtime.h"

namespace py {

const BuiltinAttribute SliceBuiltins::kAttributes[] = {
    {ID(start), Slice::kStartOffset, AttributeFlags::kReadOnly},
    {ID(stop), Slice::kStopOffset, AttributeFlags::kReadOnly},
    {ID(step), Slice::kStepOffset, AttributeFlags::kReadOnly},
    {SymbolId::kSentinelId, -1},
};

const BuiltinMethod SliceBuiltins::kBuiltinMethods[] = {
    {ID(__new__), dunderNew},
    {SymbolId::kSentinelId, nullptr},
};

RawObject SliceBuiltins::dunderNew(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object type_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfType(*type_obj)) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "'__new__' requires a type object");
  }
  Type type(&scope, *type_obj);
  Layout layout(&scope, type.instanceLayout());
  if (layout.id() != LayoutId::kSlice) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "slice.__new__ requires the slice type");
  }
  Object start(&scope, NoneType::object());
  Object stop(&scope, NoneType::object());
  Object step(&scope, NoneType::object());
  if (args.get(2).isUnbound()) {
    stop = args.get(1);
    return runtime->newSlice(start, stop, step);
  }
  start = args.get(1);
  stop = args.get(2);
  step = args.get(3);  // defaults to None
  return runtime->newSlice(start, stop, step);
}

}  // namespace py

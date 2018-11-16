#include "int-builtins.h"

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace python {

Object* builtinSmallIntegerBitLength(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 1) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Arguments args(frame, nargs);
  Object* self = args.get(0);
  if (!self->isSmallInteger()) {
    return thread->throwTypeErrorFromCString(
        "bit_length() must be called with int instance as first argument");
  }
  uword number =
      static_cast<uword>(std::abs(SmallInteger::cast(self)->value()));
  return SmallInteger::fromWord(Utils::highestBit(number));
}

Object* builtinSmallIntegerBool(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 1) {
    return thread->throwTypeErrorFromCString("not enough arguments");
  }
  Arguments args(frame, nargs);
  if (args.get(0)->isSmallInteger()) {
    return Boolean::fromBool(args.get(0) != SmallInteger::fromWord(0));
  }
  return thread->throwTypeErrorFromCString("unsupported type for __bool__");
}

Object* builtinSmallIntegerEq(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Arguments args(frame, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (self->isSmallInteger() && other->isSmallInteger()) {
    return Boolean::fromBool(self == other);
  }
  return thread->runtime()->notImplemented();
}

Object* builtinSmallIntegerInvert(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 1) {
    return thread->throwTypeErrorFromCString("not enough arguments");
  }
  Arguments args(frame, nargs);
  if (args.get(0)->isSmallInteger()) {
    SmallInteger* tos = SmallInteger::cast(args.get(0));
    return SmallInteger::fromWord(-(tos->value() + 1));
  }
  return thread->throwTypeErrorFromCString("unsupported type for __invert__");
}

Object* builtinSmallIntegerLe(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Arguments args(frame, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (self->isSmallInteger() && other->isSmallInteger()) {
    SmallInteger* left = SmallInteger::cast(self);
    SmallInteger* right = SmallInteger::cast(other);
    return Boolean::fromBool(left->value() <= right->value());
  }
  return thread->runtime()->notImplemented();
}

Object* builtinSmallIntegerFloorDiv(Thread* thread, Frame* caller, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Arguments args(caller, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (!self->isSmallInteger()) {
    return thread->throwTypeErrorFromCString(
        "__floordiv__() must be called with int instance as first argument");
  }
  word left = SmallInteger::cast(self)->value();
  if (other->isInteger()) {
    word right = Integer::cast(other)->asWord();
    if (right == 0) {
      UNIMPLEMENTED("ZeroDivisionError");
    }
    return thread->runtime()->newInteger(left / right);
  }
  return thread->runtime()->notImplemented();
}

Object* builtinSmallIntegerLt(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Arguments args(frame, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (self->isSmallInteger() && other->isSmallInteger()) {
    SmallInteger* left = SmallInteger::cast(self);
    SmallInteger* right = SmallInteger::cast(other);
    return Boolean::fromBool(left->value() < right->value());
  }
  return thread->runtime()->notImplemented();
}

Object* builtinSmallIntegerGe(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Arguments args(frame, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (self->isSmallInteger() && other->isSmallInteger()) {
    SmallInteger* left = SmallInteger::cast(self);
    SmallInteger* right = SmallInteger::cast(other);
    return Boolean::fromBool(left->value() >= right->value());
  }
  return thread->runtime()->notImplemented();
}

Object* builtinSmallIntegerGt(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Arguments args(frame, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (self->isSmallInteger() && other->isSmallInteger()) {
    SmallInteger* left = SmallInteger::cast(self);
    SmallInteger* right = SmallInteger::cast(other);
    return Boolean::fromBool(left->value() > right->value());
  }
  return thread->runtime()->notImplemented();
}

Object* builtinSmallIntegerMod(Thread* thread, Frame* caller, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Arguments args(caller, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (!self->isSmallInteger()) {
    return thread->throwTypeErrorFromCString(
        "__mod__() must be called with int instance as first argument");
  }
  word left = SmallInteger::cast(self)->value();
  if (other->isInteger()) {
    word right = Integer::cast(other)->asWord();
    if (right == 0) {
      UNIMPLEMENTED("ZeroDivisionError");
    }
    return thread->runtime()->newInteger(left % right);
  }
  return thread->runtime()->notImplemented();
}

Object* builtinSmallIntegerMul(Thread* thread, Frame* caller, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Arguments args(caller, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (!self->isSmallInteger()) {
    return thread->throwTypeErrorFromCString(
        "__mul__() must be called with int instance as first argument");
  }
  word left = SmallInteger::cast(self)->value();
  if (other->isInteger()) {
    word right = Integer::cast(other)->asWord();
    word product = left * right;
    if (!(left == 0 || (product / left) == right)) {
      UNIMPLEMENTED("small integer overflow");
    }
    return thread->runtime()->newInteger(product);
  }
  return thread->runtime()->notImplemented();
}

Object* builtinSmallIntegerNe(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Arguments args(frame, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (self->isSmallInteger() && other->isSmallInteger()) {
    return Boolean::fromBool(self != other);
  }
  return thread->runtime()->notImplemented();
}

Object* builtinSmallIntegerNeg(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 1) {
    return thread->throwTypeErrorFromCString("not enough arguments");
  }
  Arguments args(frame, nargs);
  SmallInteger* tos = SmallInteger::cast(args.get(0));
  return SmallInteger::fromWord(-tos->value());
}

Object* builtinSmallIntegerPos(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 1) {
    return thread->throwTypeErrorFromCString("not enough arguments");
  }
  Arguments args(frame, nargs);
  return SmallInteger::cast(args.get(0));
}

Object* builtinSmallIntegerAdd(Thread* thread, Frame* caller, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }

  Arguments args(caller, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);

  if (!self->isSmallInteger()) {
    return thread->throwTypeErrorFromCString(
        "__add__() must be called with int instance as first argument");
  }

  word left = SmallInteger::cast(self)->value();
  if (other->isInteger()) {
    word right = Integer::cast(other)->asWord();
    return thread->runtime()->newInteger(left + right);
  }
  return thread->runtime()->notImplemented();
}

Object* builtinSmallIntegerSub(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }

  Arguments args(frame, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);

  if (!self->isSmallInteger()) {
    return thread->throwTypeErrorFromCString(
        "__sub__() must be called with int instance as first argument");
  }

  word left = SmallInteger::cast(self)->value();
  if (other->isInteger()) {
    word right = Integer::cast(other)->asWord();
    return thread->runtime()->newInteger(left - right);
  }
  return thread->runtime()->notImplemented();
}

Object* builtinSmallIntegerXor(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Arguments args(frame, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (!self->isSmallInteger()) {
    return thread->throwTypeErrorFromCString(
        "__xor__() must be called with int instance as first argument");
  }
  word left = SmallInteger::cast(self)->value();
  if (other->isInteger()) {
    word right = Integer::cast(other)->asWord();
    return thread->runtime()->newInteger(left ^ right);
  }
  return thread->runtime()->notImplemented();
}

}  // namespace python

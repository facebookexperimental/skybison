#include "bytecode.h"

namespace python {

const char* const kBytecodeNames[] = {
#define NAME(name, value, handler) #name,
    FOREACH_BYTECODE(NAME)
#undef NAME
};

const CompareOp kSwappedCompareOp[] = {GT, GE, EQ, NE, LT, LE};

BytecodeOp nextBytecodeOp(const MutableBytes& bytecode, word* index) {
  word i = *index;
  Bytecode bc = static_cast<Bytecode>(bytecode.byteAt(i++));
  int32_t arg = bytecode.byteAt(i++);
  while (bc == Bytecode::EXTENDED_ARG) {
    bc = static_cast<Bytecode>(bytecode.byteAt(i++));
    arg = (arg << kBitsPerByte) | bytecode.byteAt(i++);
  }
  DCHECK(i - *index <= 8, "EXTENDED_ARG-encoded arg must fit in int32_t");
  *index = i;
  return BytecodeOp{bc, arg};
}

}  // namespace python

# Copyright (c) Facebook, Inc. and its affiliates. (http://www.facebook.com)
async def foo():
    l = (i async for i in gen())
    return [i for i in l]

# EXPECTED:
[
    ...,
    CODE_START('foo'),
    ...,
    GET_AITER(0),
    CALL_FUNCTION(1),
    ...,
    GET_ITER(0),
    CALL_FUNCTION(1),
    ...,
    CODE_START('<genexpr>'),
    LOAD_FAST('.0'),
    SETUP_EXCEPT(Block(2)),
    GET_ANEXT(0),
    LOAD_CONST(None),
    YIELD_FROM(0),
    ...,
    POP_BLOCK(0),
    JUMP_FORWARD(Block(3)),
    DUP_TOP(0),
    LOAD_GLOBAL('StopAsyncIteration'),
    COMPARE_OP('exception match'),
    POP_JUMP_IF_TRUE(Block(5)),
    END_FINALLY(0),
    ...,
    YIELD_VALUE(0),
    POP_TOP(0),
    JUMP_ABSOLUTE(Block(1)),
    POP_TOP(0),
    POP_TOP(0),
    POP_TOP(0),
    POP_EXCEPT(0),
    POP_TOP(0),
    ...,
    CODE_START('<listcomp>'),
    ...,
]

# Copyright (c) Facebook, Inc. and its affiliates. (http://www.facebook.com)
fun(a, *c, kw=1)
# EXPECTED:
[
    ...,
    BUILD_TUPLE(1),
    ...,
    BUILD_TUPLE_UNPACK_WITH_CALL(2),
    ...,
    BUILD_MAP(1),
    CALL_FUNCTION_EX(1),
    ...,
]

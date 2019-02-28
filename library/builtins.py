#!/usr/bin/env python3
"""Built-in classes, functions, and constants."""


# This is the patch decorator, injected by our boot process. flake8 has no
# knowledge about its definition and will complain without this gross circular
# helper here.
_patch = _patch  # noqa: F821
_UnboundValue = _UnboundValue  # noqa: F821
_stdout = _stdout  # noqa: F821


class function(bootstrap=True):
    def __repr__(self):
        # TODO(T32655200): Replace 0x with #x when formatting language is
        # implemented
        return f"<function {self.__name__} at 0x{_address(self)}>"

    def __call__(self, *args, **kwargs):
        return self(*args, **kwargs)

    def __get__(self, instance, owner):
        pass


def format(obj, fmt_spec):
    if not isinstance(fmt_spec, str):
        raise TypeError(
            f"fmt_spec must be str instance, not '{type(fmt_spec).__name__}'"
        )
    result = obj.__format__(fmt_spec)
    if not isinstance(result, str):
        raise TypeError(
            f"__format__ must return str instance, not '{type(result).__name__}'"
        )
    return result


# This needs to be patched before type is patched or the call to isinstance in
# type.__call__ won't properly check isinstance's arguments.
@_patch
def isinstance(obj, ty):
    pass


class type(bootstrap=True):
    def __call__(self, *args, **kwargs):
        if not isinstance(self, type):
            raise TypeError("self must be a type instance")
        obj = self.__new__(self, *args, **kwargs)
        # Special case for getting the type of an object with type(obj).
        if self == type and len(args) == 1 and len(kwargs) == 0:
            return obj
        if self.__init__(obj, *args, **kwargs) is not None:
            raise TypeError(f"{self.__name__}.__init__ returned non None")
        return obj

    def __new__(cls, name_or_object, bases=_UnboundValue, dict=_UnboundValue):
        pass

    # Not a patch; just empty
    def __init__(self, name_or_object, bases=_UnboundValue, dict=_UnboundValue):
        pass

    def __repr__(self):
        # TODO(T32810595): Handle modules, qualname
        return f"<class '{self.__name__}'>"


class object(bootstrap=True):  # noqa: E999
    def __new__(cls, *args, **kwargs):
        pass

    def __init__(self, *args, **kwargs):
        pass

    def __str__(self):
        return self.__repr__()

    def __format__(self, format_spec):
        if format_spec != "":
            raise TypeError("format_spec must be empty string")
        return str(self)

    def __hash__(self):
        pass

    def __repr__(self):
        pass


class bool(bootstrap=True):
    def __new__(cls, val=False):
        pass

    def __repr__(self):
        return "True" if self else "False"

    def __str__(self) -> str:  # noqa: T484
        return bool.__repr__(self)


class coroutine(bootstrap=True):
    def send(self, value):
        pass


class float(bootstrap=True):
    def __abs__(self) -> float:
        pass

    def __add__(self, n: float) -> float:
        pass

    def __bool__(self) -> bool:
        pass

    def __eq__(self, n: float) -> bool:  # noqa: T484
        pass

    def __float__(self) -> float:
        pass

    def __ge__(self, n: float) -> bool:
        pass

    def __gt__(self, n: float) -> bool:
        pass

    def __le__(self, n: float) -> bool:
        pass

    def __lt__(self, n: float) -> bool:
        pass

    def __mul__(self, n: float) -> float:
        pass

    def __ne__(self, n: float) -> float:  # noqa: T484
        if not isinstance(self, float):
            raise TypeError(
                f"'__ne__' requires a 'float' object "
                f"but received a '{self.__class__.__name__}'"
            )
        if not isinstance(n, float) and not isinstance(n, int):
            return NotImplemented
        return not float.__eq__(self, n)  # noqa: T484

    def __neg__(self) -> float:
        pass

    def __new__(cls, arg=0.0) -> float:
        pass

    def __pow__(self, y, z=_UnboundValue) -> float:
        pass

    def __radd__(self, n: float) -> float:
        # The addition for floating point numbers is commutative:
        # https://en.wikipedia.org/wiki/Floating-point_arithmetic#Accuracy_problems.
        # Note: Handling NaN payloads may vary depending on the hardware, but nobody
        # seems to depend on it due to such variance.
        return float.__add__(self, n)

    def __repr__(self) -> str:  # noqa: T484
        pass

    def __rmul__(self, n: float) -> float:
        # The multiplication for floating point numbers is commutative:
        # https://en.wikipedia.org/wiki/Floating-point_arithmetic#Accuracy_problems
        return float.__mul__(self, n)

    def __rsub__(self, n: float) -> float:
        # n - self == -self + n.
        return float.__neg__(self).__add__(n)

    def __rtruediv__(self, n: float) -> float:
        pass

    def __truediv__(self, n: float) -> float:
        pass

    def __sub__(self, n: float) -> float:
        pass


class generator(bootstrap=True):
    def __iter__(self):
        pass

    def __next__(self):
        pass

    def send(self, value):
        pass


class property(bootstrap=True):
    def __new__(cls, fget=None, fset=None, fdel=None, doc=None):
        pass

    def __init__(self, fget=None, fset=None, fdel=None):
        pass

    def deleter(self, fn):
        pass

    def setter(self, fn):
        pass

    def getter(self, fn):
        pass

    def __get__(self, instance, owner):
        pass

    def __set__(self, instance, value):
        pass


class int(bootstrap=True):
    def __new__(cls, n=0, base=_UnboundValue):
        pass

    def __abs__(self) -> int:
        pass

    def __add__(self, n: int) -> int:
        pass

    def __and__(self, n: int) -> int:
        pass

    def __bool__(self) -> bool:
        pass

    def __ceil__(self) -> int:
        pass

    def __eq__(self, n: int) -> bool:  # noqa: T484
        pass

    def __divmod__(self, n: int):
        pass

    def __float__(self) -> float:
        pass

    def __floor__(self) -> int:
        pass

    def __floordiv__(self, n: int) -> int:
        pass

    def __ge__(self, n: int) -> bool:
        pass

    def __gt__(self, n: int) -> bool:
        pass

    def __index__(self) -> int:
        pass

    def __int__(self) -> int:
        pass

    def __invert__(self) -> int:
        pass

    def __le__(self, n: int) -> bool:
        pass

    def __lshift__(self, n: int) -> int:
        pass

    def __lt__(self, n: int) -> bool:
        pass

    def __mod__(self, n: int) -> int:
        pass

    def __mul__(self, n: int) -> int:
        pass

    def __ne__(self, n: int) -> int:  # noqa: T484
        pass

    def __neg__(self) -> int:
        pass

    def __or__(self, n: int) -> int:
        pass

    def __pos__(self) -> int:
        pass

    def __radd__(self, n: int) -> int:
        if not isinstance(self, int):
            raise TypeError("'__radd__' requires a 'int' object")
        if not isinstance(n, int):
            return NotImplemented
        return n.__add__(self)

    def __rand__(self, n: int) -> int:
        if not isinstance(self, int):
            raise TypeError("'__rand__' requires a 'int' object")
        if not isinstance(n, int):
            return NotImplemented
        return int.__and__(n, self)

    def __repr__(self) -> str:  # noqa: T484
        pass

    def __rdivmod__(self, n: int) -> int:
        if not isinstance(self, int):
            raise TypeError("'__rdivmod__' requires a 'int' object")
        if not isinstance(n, int):
            return NotImplemented
        return int.__divmod__(n, self)  # noqa: T484

    def __rfloordiv__(self, n: int) -> int:
        if not isinstance(self, int):
            raise TypeError("'__rfloordiv__' requires a 'int' object")
        if not isinstance(n, int):
            return NotImplemented
        return int.__floordiv__(n, self)  # noqa: T484

    def __rlshift__(self, n: int) -> int:
        if not isinstance(self, int):
            raise TypeError("'__rlshift__' requires a 'int' object")
        if not isinstance(n, int):
            return NotImplemented
        return int.__lshift__(n, self)

    def __rmod__(self, n: int) -> int:
        if not isinstance(self, int):
            raise TypeError("'__rmod__' requires a 'int' object")
        if not isinstance(n, int):
            return NotImplemented
        return int.__mod__(n, self)  # noqa: T484

    def __rmul__(self, n: int) -> int:
        if not isinstance(self, int):
            raise TypeError("'__rmul__' requires a 'int' object")
        if not isinstance(n, int):
            return NotImplemented
        return int.__mul__(n, self)

    def __ror__(self, n: int) -> int:
        if not isinstance(self, int):
            raise TypeError("'__ror__' requires a 'int' object")
        if not isinstance(n, int):
            return NotImplemented
        return int.__or__(n, self)

    def __rpow__(self, n: int, *, mod=None):
        if not isinstance(self, int):
            raise TypeError("'__rpow__' requires a 'int' object")
        if not isinstance(n, int):
            return NotImplemented
        return int.__pow__(n, self, mod=mod)  # noqa: T484

    def __round__(self) -> int:
        pass

    def __rrshift__(self, n: int) -> int:
        if not isinstance(self, int):
            raise TypeError("'__rrshift__' requires a 'int' object")
        if not isinstance(n, int):
            return NotImplemented
        return int.__rshift__(n, self)

    def __rshift__(self, n: int) -> int:
        pass

    def __str__(self) -> str:  # noqa: T484
        pass

    def __rsub__(self, n: int) -> int:
        if not isinstance(self, int):
            raise TypeError("'__rsub__' requires a 'int' object")
        if not isinstance(n, int):
            return NotImplemented
        return int.__sub__(n, self)

    def __rtruediv__(self, n: int) -> int:
        if not isinstance(self, int):
            raise TypeError("'__rtruediv__' requires a 'int' object")
        if not isinstance(n, int):
            return NotImplemented
        return int.__truediv__(n, self)  # noqa: T484

    def __rxor__(self, n: int) -> int:
        if not isinstance(self, int):
            raise TypeError("'__rxor__' requires a 'int' object")
        if not isinstance(n, int):
            return NotImplemented
        return int.__xor__(n, self)

    def __sub__(self, n: int) -> int:
        pass

    def __truediv__(self, other):
        pass

    def __trunc__(self) -> int:
        pass

    def __xor__(self, n: int) -> int:
        pass

    def bit_length(self) -> int:
        pass

    def conjugate(self) -> int:
        pass

    @property
    def denominator(self) -> int:
        return 1  # noqa: T484

    @property
    def imag(self) -> int:
        return 0  # noqa: T484

    numerator = property(__int__)

    real = property(__int__)

    def to_bytes(self, length, byteorder, signed=False):
        pass


class UnicodeError(bootstrap=True):  # noqa: B903
    def __init__(self, *args):
        self.args = args


def _index(num):
    if not isinstance(num, int):
        try:
            # TODO(T41077650): Truncate the result of __index__ to Py_ssize_t
            return num.__index__()
        except AttributeError:
            raise TypeError(
                f"'{type(num).__name__}' object cannot be interpreted as" " an integer"
            )
    return num


class UnicodeDecodeError(UnicodeError, bootstrap=True):
    def __init__(self, encoding, obj, start, end, reason):
        super(UnicodeDecodeError, self).__init__(encoding, obj, start, end, reason)
        if not isinstance(encoding, str):
            raise TypeError(f"argument 1 must be str, not {type(encoding).__name__}")
        self.encoding = encoding
        self.start = _index(start)
        self.end = _index(end)
        if not isinstance(reason, str):
            raise TypeError(f"argument 5 must be str, not {type(reason).__name__}")
        self.reason = reason
        # TODO(T38246066): Replace with a check for the buffer protocol
        if not isinstance(obj, (bytes, bytearray)):
            raise TypeError(
                f"a bytes-like object is required, not '{type(obj).__name__}'"
            )
        self.object = obj


class UnicodeEncodeError(UnicodeError, bootstrap=True):
    def __init__(self, encoding, obj, start, end, reason):
        super(UnicodeEncodeError, self).__init__(encoding, obj, start, end, reason)
        if not isinstance(encoding, str):
            raise TypeError(f"argument 1 must be str, not {type(encoding).__name__}")
        self.encoding = encoding
        self.start = _index(start)
        self.end = _index(end)
        if not isinstance(reason, str):
            raise TypeError(f"argument 5 must be str, not {type(reason).__name__}")
        self.reason = reason
        # TODO(T38246066): Replace with a check for the buffer protocol
        if not isinstance(obj, (bytes, bytearray)):
            raise TypeError(
                f"a bytes-like object is required, not '{type(obj).__name__}'"
            )
        self.object = obj


class UnicodeTranslateError(UnicodeError, bootstrap=True):
    def __init__(self, obj, start, end, reason):
        super(UnicodeTranslateError, self).__init__(obj, start, end, reason)
        if not isinstance(obj, str):
            raise TypeError(f"argument 1 must be str, not {type(obj).__name__}")
        self.object = obj
        self.start = _index(start)
        self.end = _index(end)
        if not isinstance(reason, str):
            raise TypeError(f"argument 4 must be str, not {type(reason).__name__}")
        self.reason = reason


class ImportError(bootstrap=True):
    def __init__(self, *args, name=None, path=None):
        # TODO(mpage): Call super once we have EX calling working for built-in methods
        self.args = args
        if len(args) == 1:
            self.msg = args[0]
        self.name = name
        self.path = path


class BaseException(bootstrap=True):
    def __init__(self, *args):
        pass

    def __str__(self):
        if not isinstance(self, BaseException):
            raise TypeError("not a BaseException object")
        if not self.args:
            return ""
        if len(self.args) == 1:
            return str(self.args[0])
        return str(self.args)

    def __repr__(self):
        if not isinstance(self, BaseException):
            raise TypeError("not a BaseException object")
        return f"{self.__class__.__name__}{self.args!r}"


class KeyError(bootstrap=True):
    def __str__(self):
        if isinstance(self.args, tuple) and len(self.args) == 1:
            return repr(self.args[0])
        return super(KeyError, self).__str__()


class bytearray(bootstrap=True):
    def __add__(self, other) -> bytearray:
        pass

    def __getitem__(self, key):  # -> Union[int, bytearray]
        pass

    def __iadd__(self, other) -> bytearray:
        pass

    def __init__(
        self, source=_UnboundValue, encoding=_UnboundValue, errors=_UnboundValue
    ):
        pass

    def __new__(
        cls, source=_UnboundValue, encoding=_UnboundValue, errors=_UnboundValue
    ):
        pass

    def __repr__(self):
        pass

    def join(self, iterable) -> bytearray:
        if not isinstance(self, bytearray):
            raise TypeError("'join' requires a 'bytearray' object")
        result = _bytearray_join(self, iterable)
        if result is not None:
            return result
        items = [x for x in iterable]
        return _bytearray_join(self, items)


class bytes(bootstrap=True):
    def __add__(self, other: bytes) -> bytes:
        pass

    def __eq__(self, other):
        pass

    def __ge__(self, other):
        pass

    def __getitem__(self, key):
        pass

    def __gt__(self, other):
        pass

    def __le__(self, other):
        pass

    def __len__(self) -> int:
        pass

    def __lt__(self, other):
        pass

    def __ne__(self, other):
        pass

    def __new__(
        cls, source=_UnboundValue, encoding=_UnboundValue, errors=_UnboundValue
    ):
        pass

    def __repr__(self) -> str:  # noqa: T484
        pass

    def join(self, iterable) -> bytes:
        if not isinstance(self, bytes):
            raise TypeError("'join' requires a 'bytes' object")
        result = _bytes_join(self, iterable)
        if result is not None:
            return result
        items = [x for x in iterable]
        return _bytes_join(self, items)


class tuple(bootstrap=True):
    def __new__(cls, iterable=_UnboundValue):
        pass

    def __add__(self, other):
        pass

    def __eq__(self, other):
        pass

    def __getitem__(self, index):
        pass

    def __iter__(self):
        pass

    def __len__(self):
        pass

    def __mul__(self, other):
        pass

    def __repr__(self):
        num_elems = len(self)
        if num_elems == 1:
            return f"({self[0]!r},)"
        output = "("
        i = 0
        while i < num_elems:
            if i != 0:
                output += ", "
            output += repr(self[i])
            i += 1
        return output + ")"


class tuple_iterator(bootstrap=True):
    def __iter__(self):
        pass

    def __next__(self):
        pass

    def __length_hint__(self):
        pass


class list(bootstrap=True):
    def __new__(cls, iterable=()):
        pass

    def __init__(self, iterable=()):
        self.extend(iterable)

    def __add__(self, other):
        pass

    def __contains__(self, value):
        pass

    def __delitem__(self, key):
        pass

    def __getitem__(self, key):
        pass

    def __iter__(self):
        pass

    def __len__(self):
        pass

    def __mul__(self, other):
        pass

    def __setitem__(self, key, value):
        pass

    def __repr__(self):
        return "[" + ", ".join([i.__repr__() for i in self]) + "]"

    def append(self, other):
        pass

    def extend(self, other):
        pass

    def insert(self, index, value):
        pass

    def pop(self, index=_UnboundValue):
        pass

    def remove(self, value):
        pass


class list_iterator(bootstrap=True):
    def __iter__(self):
        pass

    def __next__(self):
        pass

    def __length_hint__(self):
        pass


class range(bootstrap=True):
    def __new__(cls, start_or_stop, stop=_UnboundValue, step=_UnboundValue):
        pass

    def __iter__(self):
        pass


class range_iterator(bootstrap=True):
    def __iter__(self):
        pass

    def __next__(self):
        pass

    def __length_hint__(self):
        pass


class slice(bootstrap=True):
    def __new__(cls, start_or_stop, stop=_UnboundValue, step=None):
        pass


class smallint(bootstrap=True):
    pass


class str(bootstrap=True):
    def __new__(cls, obj="", encoding=_UnboundValue, errors=_UnboundValue):
        if not isinstance(cls, type):
            raise TypeError("cls is not a type object")
        if not issubclass(cls, str):
            raise TypeError("cls is not a subtype of str")
        if cls != str:
            # TODO(T40529650): Add an unimplemented function
            raise NotImplementedError("__new__ with subtype of str")
        if type(obj) is str and obj == "":
            return obj
        if encoding != _UnboundValue or errors != _UnboundValue:
            # TODO(T40529650): Add an unimplemented function
            raise NotImplementedError("str encoding not supported yet")
        result = obj.__str__()
        if not isinstance(result, str):
            raise TypeError("__str__ returned non-str instance")
        return result

    def __add__(self, other):
        pass

    def __eq__(self, other):
        pass

    def __ge__(self, other):
        pass

    def __getitem__(self, index):
        pass

    def __gt__(self, other):
        pass

    def __iter__(self):
        pass

    def __le__(self, other):
        pass

    def __lt__(self, other):
        pass

    def __mod__(self, other):
        pass

    def __ne__(self, other):
        pass

    def __str__(self):
        return self

    def __len__(self) -> int:
        pass

    def __repr__(self):
        pass

    def partition(self, sep):
        if not isinstance(self, str):
            raise TypeError(
                f"descriptor 'partition' requires a 'str' object but received a {type(self).__name__}"
            )
        if not isinstance(sep, str):
            raise TypeError(f"must be str, not {type(sep).__name__}")
        sep_len = len(sep)
        if not sep_len:
            raise ValueError("empty separator")
        sep_0 = sep[0]
        i = 0
        str_len = len(self)
        while i < str_len:
            if self[i] == sep_0:
                j = 1
                while j < sep_len:
                    if i + j >= str_len or self[i + j] != sep[j]:
                        break
                    j += 1
                else:
                    return (self[:i], sep, self[i + j :])
            i += 1
        return (self, "", "")

    def split(self, sep=None, maxsplit=-1):
        if maxsplit == 0:
            return self
        # If the separator is not specified, split on all whitespace characters.
        if sep is None:
            raise NotImplementedError("Splitting on whitespace not yet implemented")
        if not isinstance(sep, str):
            raise TypeError("must be str or None")
        sep_len = len(sep)
        if sep_len == 0:
            raise ValueError("empty separator")

        def strcmp(cmp, other, start):
            """Returns True if other is in cmp at the position start"""
            i = 0
            cmp_len = len(cmp)
            other_len = len(other)
            # If the other string is longer than the rest of the current string,
            # then it is not a match.
            if start + other_len > cmp_len:
                return False
            while i < other_len and start + i < cmp_len:
                if cmp[start + i] != other[i]:
                    return False
                i += 1
            return True

        # TODO(dulinr): This path uses random-access indices on strings, which
        # is not efficient for UTF-8 strings.
        parts = []
        i = 0
        str_len = len(self)
        # The index of the string starting after the last match.
        last_match = 0
        while i < str_len:
            if strcmp(self, sep, i):
                parts.append(self[last_match:i])
                last_match = i + sep_len
                # If we've hit the max number of splits, return early.
                maxsplit -= 1
                if maxsplit == 0:
                    break
                i += sep_len
            else:
                i += 1
        parts.append(self[last_match:])
        return parts

    def startswith(self, prefix, start=0, end=None):
        def real_bounds_from_slice_bounds(start, end, length):
            if start < 0:
                start = length + start
            if start < 0:
                start = 0
            if start > length:
                start = length

            if end is None or end > length:
                end = length
            if end < 0:
                end = length + end
            if end < 0:
                end = 0
            return start, end

        def prefix_match(cmp, prefix, start, end):
            if not isinstance(prefix, str):
                raise TypeError("startswith prefix must be a str")
            prefix_len = len(prefix)
            # If the prefix is longer than the string its comparing against, it
            # can't be a match.
            if end - start < prefix_len:
                return False
            end = start + prefix_len

            # Iterate through cmp from [start, end), checking against
            # the characters in the suffix.
            i = 0
            while i < prefix_len:
                if cmp[start + i] != prefix[i]:
                    return False
                i += 1
            return True

        str_len = len(self)
        start, end = real_bounds_from_slice_bounds(start, end, str_len)
        if not isinstance(prefix, tuple):
            return prefix_match(self, prefix, start, end)

        for pref in prefix:
            if prefix_match(self, pref, start, end):
                return True
        return False

    def endswith(self, suffix, start=0, end=None):
        def real_bounds_from_slice_bounds(start, end, length):
            if start < 0:
                start = length + start
            if start < 0:
                start = 0
            if start > length:
                start = length

            if end is None or end > length:
                end = length
            if end < 0:
                end = length + end
            if end < 0:
                end = 0
            return start, end

        def suffix_match(cmp, sfx, start, end):
            if not isinstance(sfx, str):
                raise TypeError("endswith suffix must be a str")
            sfx_len = len(sfx)
            # If the suffix is longer than the string its comparing against, it
            # can't be a match.
            if end - start < sfx_len:
                return False
            start = end - sfx_len

            # Iterate through cmp from [end - sfx_len, end), checking against
            # the characters in the suffix.
            i = 0
            while i < sfx_len:
                if cmp[start + i] != sfx[i]:
                    return False
                i += 1
            return True

        str_len = len(self)
        start, end = real_bounds_from_slice_bounds(start, end, str_len)
        if not isinstance(suffix, tuple):
            return suffix_match(self, suffix, start, end)

        for suf in suffix:
            if suffix_match(self, suf, start, end):
                return True
        return False

    # TODO(T37437993): Write in C++
    def rsplit(self, sep=None, maxsplit=-1):
        return [s[::-1] for s in self[::-1].split(sep[::-1], maxsplit)[::-1]]

    # TODO(T37438017): Write in C++
    def rpartition(self, sep):
        before, itself, after = self[::-1].partition(sep[::-1])[::-1]
        return before[::-1], itself[::-1], after[::-1]

    def join(self, items) -> str:
        pass

    def lower(self):
        pass

    def lstrip(self, other=None):
        pass

    def strip(self, other=None):
        pass

    def rstrip(self, other=None):
        pass


class str_iterator(bootstrap=True):
    def __iter__(self):
        pass

    def __next__(self):
        pass

    def __length_hint__(self):
        pass


class dict(bootstrap=True):
    def __contains__(self, key) -> bool:
        return self.get(key, _UnboundValue) is not _UnboundValue  # noqa: T484

    def __delitem__(self, key):
        pass

    def __eq__(self, other):
        pass

    def __len__(self):
        pass

    def __new__(cls):
        pass

    def __getitem__(self, key):
        result = self.get(key, _UnboundValue)
        if result is _UnboundValue:
            raise KeyError(key)
        return result

    def __setitem__(self, key, value):
        pass

    def __iter__(self):
        pass

    def __repr__(self):
        kwpairs = [f"{key!r}: {self[key]!r}" for key in self.keys()]
        return "{" + ", ".join(kwpairs) + "}"

    def get(self, key, default=None):
        pass

    def items(self):
        pass

    def keys(self):
        pass

    def update(self, other):
        pass

    def values(self):
        pass


class dict_itemiterator(bootstrap=True):
    def __iter__(self):
        pass

    def __next__(self):
        pass

    def __length_hint__(self):
        pass


class dict_items(bootstrap=True):
    def __iter__(self):
        pass


class dict_keyiterator(bootstrap=True):
    def __iter__(self):
        pass

    def __next__(self):
        pass

    def __length_hint__(self):
        pass


class dict_keys(bootstrap=True):
    def __iter__(self):
        pass


class dict_valueiterator(bootstrap=True):
    def __iter__(self):
        pass

    def __next__(self):
        pass

    def __length_hint__(self):
        pass


class dict_values(bootstrap=True):
    def __iter__(self):
        pass


class module(bootstrap=True):
    def __new__(cls, name):
        pass


class frozenset(bootstrap=True):
    def __new__(cls, iterable=_UnboundValue):
        pass

    def __and__(self, other):
        pass

    def __contains__(self, value):
        pass

    def __eq__(self, other):
        pass

    def __ge__(self, other):
        pass

    def __gt__(self, other):
        pass

    def __iter__(self):
        pass

    def __ne__(self, other):
        pass

    def __le__(self, other):
        pass

    def __len__(self):
        pass

    def __lt__(self, other):
        pass

    def intersection(self, other):
        pass

    def isdisjoint(self, other):
        pass


class set(bootstrap=True):
    def __new__(cls, iterable=()):
        pass

    def __init__(self, iterable=()):
        pass

    def __repr__(self):
        return f"{{{', '.join([item.__repr__() for item in self])}}}"

    def __and__(self, other):
        pass

    def __contains__(self, value):
        pass

    def __eq__(self, other):
        pass

    def __ge__(self, other):
        pass

    def __gt__(self, other):
        pass

    def __iand__(self, other):
        pass

    def __iter__(self):
        pass

    def __ne__(self, other):
        pass

    def __le__(self, other):
        pass

    def __len__(self):
        pass

    def __lt__(self, other):
        pass

    def add(self, value):
        pass

    def intersection(self, other):
        pass

    def isdisjoint(self, other):
        pass

    def pop(self):
        pass


class set_iterator(bootstrap=True):
    def __iter__(self):
        pass

    def __next__(self):
        pass

    def __length_hint__(self):
        pass


class classmethod(bootstrap=True):
    def __new__(cls, fn):
        pass

    def __init__(self, fn):
        pass

    def __get__(self, instance, owner):
        pass


class staticmethod(bootstrap=True):
    def __new__(cls, fn):
        pass

    def __init__(self, fn):
        pass

    def __get__(self, instance, owner):
        pass


class super(bootstrap=True):
    def __new__(cls, type=_UnboundValue, type_or_obj=_UnboundValue):
        pass

    def __init__(self, type=_UnboundValue, type_or_obj=_UnboundValue):
        pass


class StopIteration(bootstrap=True):
    def __init__(self, *args, **kwargs):
        pass


class SystemExit(bootstrap=True):
    def __init__(self, *args, **kwargs):
        pass


class complex(bootstrap=True):
    def __new__(cls, real=0.0, imag=0.0):
        pass

    def __repr__(self):
        return f"({self.real}+{self.imag}j)"

    @property
    def imag(self):
        return _complex_imag(self)

    @property
    def real(self):
        return _complex_real(self)


class NoneType(bootstrap=True):
    def __new__(cls):
        pass

    def __repr__(self):
        pass


def all(iterable):
    for element in iterable:
        if not element:
            return False
    return True


def any(iterable):
    for element in iterable:
        if element:
            return True
    return False


@_patch
def _address(c):
    pass


@_patch
def _bytearray_join(self: bytearray, iterable) -> bytearray:
    pass


@_patch
def _bytes_join(self: bytes, iterable) -> bytes:
    pass


@_patch
def _complex_imag(c):
    pass


@_patch
def _complex_real(c):
    pass


@_patch
def _print_str(s, file):
    pass


def print(*args, sep=" ", end="\n", file=_stdout, flush=None):
    if args:
        _print_str(args[0].__str__(), file)
        length = len(args)
        i = 1
        while i < length:
            _print_str(sep, file)
            _print_str(args[i].__str__(), file)
            i += 1
    _print_str(end, file)
    if flush:
        raise NotImplementedError("flush in print")


@_patch
def _str_escape_non_ascii(s):
    pass


def repr(obj):
    result = type(obj).__repr__(obj)
    if not isinstance(result, str):
        raise TypeError("__repr__ returned non-string")
    return result


def ascii(obj):
    return _str_escape_non_ascii(repr(obj))


@_patch
def callable(f):
    pass


@_patch
def chr(c):
    pass


@_patch
def compile(source, filename, mode, flags=0, dont_inherit=False, optimize=-1):
    pass


@_patch
def exec(source, globals=None, locals=None):
    pass


@_patch
def getattr(obj, key, default=_UnboundValue):
    pass


# patch is not patched because that would cause a circularity problem.


@_patch
def hasattr(obj, name):
    pass


@_patch
def issubclass(obj, ty):
    pass


def len(seq):
    dunder_len = getattr(seq, "__len__", None)
    if dunder_len is None:
        raise TypeError("object has no len()")
    return dunder_len()


@_patch
def ord(c):
    pass


@_patch
def setattr(obj, name, value):
    pass


@_patch
def _structseq_setattr(obj, name, value):
    pass


@_patch
def _structseq_getattr(obj, name):
    pass


def _structseq_getitem(self, pos):
    if pos < 0 or pos >= type(self).n_fields:
        raise IndexError("index out of bounds")
    if pos < len(self):
        return self[pos]
    else:
        name = self._structseq_field_names[pos]
        return _structseq_getattr(self, name)


def _structseq_new(cls, sequence, dict={}):  # noqa B006
    seq_tuple = tuple(sequence)
    seq_len = len(seq_tuple)
    max_len = cls.n_fields
    min_len = cls.n_sequence_fields
    if seq_len < min_len:
        raise TypeError(
            f"{cls.__name__} needs at least a {min_len}-sequence ({seq_len}-sequence given)"
        )
    if seq_len > max_len:
        raise TypeError(
            f"{cls.__name__} needs at most a {max_len}-sequence ({seq_len}-sequence given)"
        )

    # Create the tuple of size min_len
    structseq = tuple.__new__(cls, seq_tuple[:min_len])

    # Fill the rest of the hidden fields
    for i in range(min_len, seq_len):
        key = cls._structseq_field_names[i]
        _structseq_setattr(structseq, key, seq_tuple[min_len])

    # Fill the remaining from the dict or set to None
    for i in range(seq_len, max_len):
        key = cls._structseq_field_names[i]
        _structseq_setattr(structseq, key, dict.get(key))

    return structseq


class _structseq_field:
    def __init__(self, name, index):
        self.name = name
        self.index = index

    def __get__(self, instance, owner):
        if self.index is not None:
            return instance[self.index]
        return _structseq_getattr(instance, self.name)

    def __set__(self, instance, value):
        raise TypeError("readonly attribute")


def _structseq_repr(self):
    if not isinstance(self, tuple):
        raise TypeError("__repr__(): self is not a tuple")
    if not hasattr(self, "n_sequence_fields"):
        raise TypeError("__repr__(): self is not a self")
    # TODO(T40273054): Iterate attributes and return field names
    tuple_values = ", ".join([i.__repr__() for i in self])
    return f"{type(self).__name__}({tuple_values})"

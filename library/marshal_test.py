#!/usr/bin/env python3
import marshal
import types
import unittest


class DumpsTest(unittest.TestCase):
    def test_dumps_with_none_returns_n(self):
        self.assertEqual(marshal.dumps(None, 2), b"N")

    def test_dumps_with_stop_iteration_returns_s(self):
        self.assertEqual(marshal.dumps(StopIteration, 2), b"S")

    def test_dumps_with_ellipsis_returns_dot(self):
        self.assertEqual(marshal.dumps(..., 2), b".")

    def test_dumps_with_false_returns_f(self):
        self.assertEqual(marshal.dumps(False, 2), b"F")

    def test_dumps_with_true_returns_t(self):
        self.assertEqual(marshal.dumps(True, 2), b"T")

    def test_dumps_with_int_zero(self):
        self.assertEqual(marshal.dumps(0, 2), b"i\x00\x00\x00\x00")

    def test_dumps_with_int_nonzero(self):
        self.assertEqual(marshal.dumps(1234, 2), b"i\xd2\x04\x00\x00")

    def test_dumps_with_int32_min(self):
        self.assertEqual(marshal.dumps(-0x7FFFFFFF - 1, 2), b"i\x00\x00\x00\x80")

    def test_dumps_with_int32_max(self):
        self.assertEqual(marshal.dumps(0x7FFFFFFF, 2), b"i\xff\xff\xff\x7f")

    def test_dumps_with_int32_min_minus_one(self):
        self.assertEqual(
            marshal.dumps(-0x7FFFFFFF - 2, 2),
            b"l\xfd\xff\xff\xff\x01\x00\x00\x00\x02\x00",
        )

    def test_dumps_with_int32_max_plus_one(self):
        self.assertEqual(
            marshal.dumps(0x7FFFFFFF + 1, 2),
            b"l\x03\x00\x00\x00\x00\x00\x00\x00\x02\x00",
        )

    def test_dumps_with_large_int(self):
        self.assertEqual(
            marshal.dumps(0x5189DD43F6FFEEBA9BD41E097, 2),
            b"l\x07\x00\x00\x00\x97`\x83z\xa6.\xf7\x7f\xf6C\xba\x13F\x01",
        )

    def test_dumps_with_negative_large_int(self):
        self.assertEqual(
            marshal.dumps(-(2 ** 70), 2),
            b"l\xfb\xff\xff\xff\x00\x00\x00\x00\x00\x00\x00\x00\x00\x04",
        )

    def test_dumps_with_float_zero(self):
        self.assertEqual(marshal.dumps(0.0, 2), b"g\x00\x00\x00\x00\x00\x00\x00\x00")

    def test_dumps_with_float_negative_zero(self):
        self.assertEqual(marshal.dumps(-0.0, 2), b"g\x00\x00\x00\x00\x00\x00\x00\x80")

    def test_dumps_with_float_nan(self):
        self.assertEqual(
            marshal.dumps(float("nan"), 2), b"g\x00\x00\x00\x00\x00\x00\xf8\x7f"
        )

    def test_dumps_with_float_infinity(self):
        self.assertEqual(
            marshal.dumps(float("infinity"), 2), b"g\x00\x00\x00\x00\x00\x00\xf0\x7f"
        )

    def test_dumps_with_float_negative_infinity(self):
        self.assertEqual(
            marshal.dumps(float("-infinity"), 2), b"g\x00\x00\x00\x00\x00\x00\xf0\xff"
        )

    def test_dumps_with_complex(self):
        self.assertEqual(
            marshal.dumps(complex(1.1, 2.2), 2),
            b"y\x9a\x99\x99\x99\x99\x99\xf1?\x9a\x99\x99\x99\x99\x99\x01@",
        )

    def test_dumps_with_bytes(self):
        self.assertEqual(marshal.dumps(b"hello", 2), b"s\x05\x00\x00\x00hello")

    def test_dumps_with_bytearray(self):
        self.assertEqual(
            marshal.dumps(bytearray(b"hello"), 2), b"s\x05\x00\x00\x00hello"
        )

    def test_dumps_with_short_ascii_str(self):
        self.assertEqual(marshal.dumps("hello", 2), b"u\x05\x00\x00\x00hello")

    def test_dumps_with_ascii_str(self):
        self.assertEqual(
            marshal.dumps("a" * 300, 2),
            b"u,\x01\x00\x00aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
            b"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
            b"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
            b"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
            b"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
        )

    def test_dumps_with_unicode_str(self):
        self.assertEqual(
            marshal.dumps(" \t\u3000\n\u202f", 2),
            b"u\t\x00\x00\x00 \t\xe3\x80\x80\n\xe2\x80\xaf",
        )

    def test_dumps_with_small_tuple(self):
        self.assertEqual(marshal.dumps((), 2), b"(\x00\x00\x00\x00")

    def test_dumps_with_tuple(self):
        self.assertEqual(
            marshal.dumps((None,) * 300, 2),
            b"(,\x01\x00\x00NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN"
            b"NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN"
            b"NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN"
            b"NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN"
            b"NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN",
        )

    def test_dumps_with_list(self):
        self.assertEqual(marshal.dumps([None, True, False], 2), b"[\x03\x00\x00\x00NTF")

    def test_dumps_with_dict(self):
        self.assertEqual(
            marshal.dumps({"hello": "world"}, 2),
            b"{u\x05\x00\x00\x00hellou\x05\x00\x00\x00world0",
        )

    def test_dumps_with_set(self):
        self.assertEqual(
            marshal.dumps({"hello"}, 2), b"<\x01\x00\x00\x00u\x05\x00\x00\x00hello"
        )

    def test_dumps_with_frozenset(self):
        self.assertEqual(
            marshal.dumps(frozenset({"hello"}), 2),
            b">\x01\x00\x00\x00u\x05\x00\x00\x00hello",
        )

    def test_dumps_with_code(self):
        code = types.CodeType(
            1,  # argcount
            2,  # kwonlyargcount
            8,  # nlocals
            4,  # stacksize
            5,  # flags
            b"foo",  # code
            (1, 2),  # consts
            ("foo", "bar"),  # names
            ("cell", "foo", "baz", "bucket"),  # varnames
            "name",  # filename
            "codename",  # name
            7,  # firstlineno
            b"lnotab",  # lnotab
            ("bucket",),  # freevars
            ("cell",),  # cellvars
        )

        self.assertEqual(
            marshal.dumps(code, 2),
            b"c\x01\x00\x00\x00\x02\x00\x00\x00\x08\x00\x00\x00\x04\x00\x00"
            b"\x00\x05\x00\x00\x00s\x03\x00\x00\x00foo(\x02\x00\x00\x00i\x01"
            b"\x00\x00\x00i\x02\x00\x00\x00(\x02\x00\x00\x00u\x03\x00\x00\x00"
            b"foou\x03\x00\x00\x00bar(\x04\x00\x00\x00u\x04\x00\x00\x00cellu"
            b"\x03\x00\x00\x00foou\x03\x00\x00\x00bazu\x06\x00\x00\x00bucket("
            b"\x01\x00\x00\x00u\x06\x00\x00\x00bucket(\x01\x00\x00\x00u\x04\x00"
            b"\x00\x00cellu\x04\x00\x00\x00nameu\x08\x00\x00\x00codename\x07"
            b"\x00\x00\x00s\x06\x00\x00\x00lnotab",
        )

    def test_dumps_with_ref_does_not_use_ref(self):
        obj = [True, False, None]
        container = (obj, obj, obj)
        self.assertEqual(
            marshal.dumps(container, 2),
            b"(\x03\x00\x00\x00[\x03\x00\x00\x00TFN[\x03\x00\x00\x00TFN[\x03"
            b"\x00\x00\x00TFN",
        )

    def test_dumps_with_int_subclass_raises_value_error(self):
        class C(int):
            pass

        instance = C()
        with self.assertRaises(ValueError) as context:
            marshal.dumps(instance, 2)
        self.assertIn("unmarshallable object", str(context.exception))

    def test_dumps_with_float_subclass_raises_value_error(self):
        class C(float):
            pass

        instance = C()
        with self.assertRaises(ValueError) as context:
            marshal.dumps(instance, 2)
        self.assertIn("unmarshallable object", str(context.exception))

    def test_dumps_with_complex_subclass_raises_value_error(self):
        class C(complex):
            pass

        instance = C()
        with self.assertRaises(ValueError) as context:
            marshal.dumps(instance, 2)
        self.assertIn("unmarshallable object", str(context.exception))

    def test_dumps_with_str_subclass_raises_value_error(self):
        class C(str):
            pass

        instance = C()
        with self.assertRaises(ValueError) as context:
            marshal.dumps(instance, 2)
        self.assertIn("unmarshallable object", str(context.exception))

    def test_dumps_with_bytearray_subclass(self):
        class C(bytearray):
            pass

        instance = C(b"hello")
        self.assertEqual(marshal.dumps(instance, 2), b"s\x05\x00\x00\x00hello")

    def test_dumps_with_bytes_subclass(self):
        class C(bytes):
            pass

        instance = C()
        instance = C(b"hello")
        self.assertEqual(marshal.dumps(instance, 2), b"s\x05\x00\x00\x00hello")

    def test_dumps_with_tuple_subclass_raises_value_error(self):
        class C(tuple):
            pass

        instance = C()
        with self.assertRaises(ValueError) as context:
            marshal.dumps(instance, 2)
        self.assertIn("unmarshallable object", str(context.exception))

    def test_dumps_with_list_subclass_raises_value_error(self):
        class C(list):
            pass

        instance = C()
        with self.assertRaises(ValueError) as context:
            marshal.dumps(instance, 2)
        self.assertIn("unmarshallable object", str(context.exception))

    def test_dumps_with_dict_subclass_raises_value_error(self):
        class C(dict):
            pass

        instance = C()
        with self.assertRaises(ValueError) as context:
            marshal.dumps(instance, 2)
        self.assertIn("unmarshallable object", str(context.exception))

    def test_dumps_with_set_subclass_raises_value_error(self):
        class C(set):
            pass

        instance = C()
        with self.assertRaises(ValueError) as context:
            marshal.dumps(instance, 2)
        self.assertIn("unmarshallable object", str(context.exception))

    def test_dumps_with_frozenset_subclass_raises_value_error(self):
        class C(frozenset):
            pass

        instance = C()
        with self.assertRaises(ValueError) as context:
            marshal.dumps(instance, 2)
        self.assertIn("unmarshallable object", str(context.exception))

    def test_dumps_with_unmarshallable_type_raises_value_error(self):
        class C:
            pass

        instance = C()
        with self.assertRaisesRegex(ValueError, "unmarshallable object"):
            marshal.dumps(instance, 2)


if __name__ == "__main__":
    unittest.main()
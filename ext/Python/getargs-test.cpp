#include "Python.h"

#include "capi-fixture.h"
#include "capi-testing.h"

namespace python {

using namespace testing;

using GetArgsExtensionApiTest = ExtensionApi;

TEST_F(GetArgsExtensionApiTest, ParseTupleOneObject) {
  PyObjectPtr pytuple(PyTuple_New(1));
  PyObject* in = PyLong_FromLong(42);
  ASSERT_NE(-1, PyTuple_SetItem(pytuple, 0, in));

  long refcnt = Py_REFCNT(in);
  PyObject* out;
  EXPECT_TRUE(PyArg_ParseTuple(pytuple, "O:xyz", &out));
  // This returns a borrowed reference, verify ref count did not change
  EXPECT_EQ(Py_REFCNT(out), refcnt);
  EXPECT_EQ(42, PyLong_AsLong(out));
}

TEST_F(GetArgsExtensionApiTest, ParseTupleMultipleObjects) {
  PyObject* pytuple = PyTuple_New(3);
  PyObject* in1 = PyLong_FromLong(111);
  PyObject* in2 = Py_None;
  PyObject* in3 = PyLong_FromLong(333);
  ASSERT_NE(-1, PyTuple_SetItem(pytuple, 0, in1));
  ASSERT_NE(-1, PyTuple_SetItem(pytuple, 1, in2));
  ASSERT_NE(-1, PyTuple_SetItem(pytuple, 2, in3));

  PyObject* out1;
  PyObject* out2;
  PyObject* out3;
  EXPECT_TRUE(PyArg_ParseTuple(pytuple, "OOO:xyz", &out1, &out2, &out3));
  EXPECT_EQ(111, PyLong_AsLong(out1));
  EXPECT_EQ(Py_None, out2);
  EXPECT_EQ(333, PyLong_AsLong(out3));
}

TEST_F(GetArgsExtensionApiTest, ParseTupleUnicodeObject) {
  PyObject* pytuple = PyTuple_New(1);
  PyObject* in1 = PyUnicode_FromString("pyro");
  ASSERT_NE(-1, PyTuple_SetItem(pytuple, 0, in1));

  PyObject* out1;
  EXPECT_TRUE(PyArg_ParseTuple(pytuple, "U:is_frozen", &out1));
  EXPECT_EQ(in1, out1);
}

TEST_F(GetArgsExtensionApiTest, ParseTupleWithWrongType) {
  PyObjectPtr pytuple(PyTuple_New(1));
  PyObjectPtr in(PyLong_FromLong(42));
  ASSERT_NE(-1, PyTuple_SetItem(pytuple, 0, in));

  PyObject* out1 = nullptr;
  EXPECT_FALSE(PyArg_ParseTuple(pytuple, "U:is_frozen", &out1));
  EXPECT_NE(PyErr_Occurred(), nullptr);
  EXPECT_EQ(out1, nullptr);
}

TEST_F(GetArgsExtensionApiTest, ParseTupleString) {
  PyObject* pytuple = PyTuple_New(2);
  PyObject* in1 = PyUnicode_FromString("hello");
  PyObject* in2 = PyUnicode_FromString("world");
  ASSERT_NE(-1, PyTuple_SetItem(pytuple, 0, in1));
  ASSERT_NE(-1, PyTuple_SetItem(pytuple, 1, in2));

  char *out1, *out2;
  EXPECT_TRUE(PyArg_ParseTuple(pytuple, "sz", &out1, &out2));
  EXPECT_STREQ("hello", out1);
  EXPECT_STREQ("world", out2);
}

TEST_F(GetArgsExtensionApiTest, ParseTupleStringFromNone) {
  PyObject* pytuple = PyTuple_New(2);
  ASSERT_NE(-1, PyTuple_SetItem(pytuple, 0, Py_None));
  ASSERT_NE(-1, PyTuple_SetItem(pytuple, 1, Py_None));

  char *out1, *out2;
  int size = 123;
  EXPECT_TRUE(PyArg_ParseTuple(pytuple, "zz#", &out1, &out2, &size));
  EXPECT_EQ(nullptr, out1);
  EXPECT_EQ(nullptr, out2);
  EXPECT_EQ(0, size);
}

TEST_F(GetArgsExtensionApiTest, ParseTupleStringWithSize) {
  PyObject* pytuple = PyTuple_New(2);
  PyObject* in1 = PyUnicode_FromString("hello");
  PyObject* in2 = PyUnicode_FromString("cpython");
  ASSERT_NE(-1, PyTuple_SetItem(pytuple, 0, in1));
  ASSERT_NE(-1, PyTuple_SetItem(pytuple, 1, in2));

  char *out1, *out2;
  int size1, size2;
  EXPECT_TRUE(PyArg_ParseTuple(pytuple, "s#z#", &out1, &size1, &out2, &size2));
  EXPECT_STREQ("hello", out1);
  EXPECT_EQ(5, size1);
  EXPECT_STREQ("cpython", out2);
  EXPECT_EQ(7, size2);
}

TEST_F(GetArgsExtensionApiTest, ParseTupleNumbers) {
  const int k_ints = 11;
  PyObject* pytuple = PyTuple_New(k_ints);
  for (int i = 0; i < k_ints; i++) {
    ASSERT_EQ(0, PyTuple_SetItem(pytuple, i, PyLong_FromLong(123 + i)));
  }

  unsigned char nb, n_b;
  short int nh;
  unsigned short int n_h;
  int ni;
  unsigned int n_i;
  long int nl;
  unsigned long nk;
  long long n_l;
  unsigned long long n_k;
  Py_ssize_t nn;

  EXPECT_TRUE(PyArg_ParseTuple(pytuple, "bBhHiIlkLKn", &nb, &n_b, &nh, &n_h,
                               &ni, &n_i, &nl, &nk, &n_l, &n_k, &nn));
  EXPECT_EQ(123, nb);
  EXPECT_EQ(124, n_b);
  EXPECT_EQ(125, nh);
  EXPECT_EQ(126, n_h);
  EXPECT_EQ(127, ni);
  EXPECT_EQ(128U, n_i);
  EXPECT_EQ(129, nl);
  EXPECT_EQ(130UL, nk);
  EXPECT_EQ(131, n_l);
  EXPECT_EQ(132ULL, n_k);
  EXPECT_EQ(133, nn);
}

TEST_F(GetArgsExtensionApiTest, ParseTupleOptionalPresent) {
  PyObjectPtr pytuple(PyTuple_New(1));
  ASSERT_NE(-1, PyTuple_SetItem(pytuple, 0, PyLong_FromLong(111)));

  PyObject* out = nullptr;
  EXPECT_TRUE(PyArg_ParseTuple(pytuple, "|O", &out));
  ASSERT_NE(out, nullptr);
  EXPECT_EQ(111, PyLong_AsLong(out));
}

TEST_F(GetArgsExtensionApiTest, ParseTupleOptionalNotPresent) {
  PyObjectPtr pytuple(PyTuple_New(0));

  PyObject* out = nullptr;
  EXPECT_TRUE(PyArg_ParseTuple(pytuple, "|O", &out));
  ASSERT_EQ(out, nullptr);
}

TEST_F(GetArgsExtensionApiTest, ParseTupleObjectWithCorrectType) {
  PyObjectPtr pytuple(PyTuple_New(1));
  PyObject* in = PyLong_FromLong(111);
  PyTypeObject* type = Py_TYPE(in);
  ASSERT_NE(-1, PyTuple_SetItem(pytuple, 0, in));

  PyObject* out = nullptr;
  EXPECT_TRUE(PyArg_ParseTuple(pytuple, "O!", type, &out));

  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(111, PyLong_AsLong(out));
}

TEST_F(GetArgsExtensionApiTest, ParseTupleObjectWithIncorrectType) {
  PyObjectPtr pytuple(PyTuple_New(1));
  PyObject* in = PyLong_FromLong(111);
  PyTypeObject* type = Py_TYPE(pytuple);
  ASSERT_NE(-1, PyTuple_SetItem(pytuple, 0, in));

  PyObject* out = nullptr;
  EXPECT_FALSE(PyArg_ParseTuple(pytuple, "O!", type, &out));

  EXPECT_NE(PyErr_Occurred(), nullptr);
  EXPECT_EQ(out, nullptr);
}

TEST_F(GetArgsExtensionApiTest, ParseTupleObjectWithConverter) {
  using converter_func = void (*)(PyObject*, void*);
  converter_func parseTupleConverter = [](PyObject* ptr, void* out) {
    *static_cast<int*>(out) = 1 + PyLong_AsLong(ptr);
  };

  PyObjectPtr pytuple(PyTuple_New(1));
  ASSERT_NE(-1, PyTuple_SetItem(pytuple, 0, PyLong_FromLong(111)));

  int out = 0;
  EXPECT_TRUE(PyArg_ParseTuple(pytuple, "O&", parseTupleConverter,
                               static_cast<void*>(&out)));
  EXPECT_EQ(out, 112);
}

TEST_F(GetArgsExtensionApiTest, OldStyleParseWithInt) {
  PyObjectPtr pylong(PyLong_FromLong(666));
  int n = 0;
  EXPECT_TRUE(PyArg_Parse(pylong, "i", &n));
  EXPECT_EQ(n, 666);
}

}  // namespace python

#include "cpython-data.h"
#include "cpython-func.h"
#include "cpython-types.h"

extern "C" PyObject* PyInit__ast();
extern "C" PyObject* PyInit__blake2();
extern "C" PyObject* PyInit__bz2();
extern "C" PyObject* PyInit__capsule();
extern "C" PyObject* PyInit__compile();
extern "C" PyObject* PyInit__hashlib();
extern "C" PyObject* PyInit__locale();
extern "C" PyObject* PyInit__lzma();
extern "C" PyObject* PyInit__multiprocessing();
extern "C" PyObject* PyInit__myreadline();
extern "C" PyObject* PyInit__posixsubprocess();
extern "C" PyObject* PyInit__random();
extern "C" PyObject* PyInit__socket();
extern "C" PyObject* PyInit__sha3();
extern "C" PyObject* PyInit__sre();
extern "C" PyObject* PyInit__ssl();
extern "C" PyObject* PyInit__stat();
extern "C" PyObject* PyInit__stentry();
extern "C" PyObject* PyInit__struct();
extern "C" PyObject* PyInit_atexit();
extern "C" PyObject* PyInit_binascii();
extern "C" PyObject* PyInit_errno();
extern "C" PyObject* PyInit_fcntl();
extern "C" PyObject* PyInit_grp();
extern "C" PyObject* PyInit_math();
extern "C" PyObject* PyInit_mmap();
extern "C" PyObject* PyInit_posix();
extern "C" PyObject* PyInit_pwd();
extern "C" PyObject* PyInit_select();
extern "C" PyObject* PyInit_syslog();
extern "C" PyObject* PyInit_termios();
extern "C" PyObject* PyInit_time();
extern "C" PyObject* PyInit_zlib();

// _empty module to test loading from init tab
PyObject* PyInit__empty() {
  static PyModuleDef def;
  def = {};
  def.m_name = "_empty";
  return PyModule_Create(&def);
}

// clang-format off
struct _inittab _PyImport_Inittab[] = {
    {"_ast", PyInit__ast},
    {"_blake2", PyInit__blake2},
    {"_bz2", PyInit__bz2},
    {"_capsule", PyInit__capsule},
    {"_compile", PyInit__compile},
    {"_empty", PyInit__empty},
    {"_hashlib", PyInit__hashlib},
    {"_locale", PyInit__locale},
    {"_lzma", PyInit__lzma},
    {"_multiprocessing", PyInit__multiprocessing},
    {"_myreadline", PyInit__myreadline},
    {"_posixsubprocess", PyInit__posixsubprocess},
    {"_random", PyInit__random},
    {"_socket", PyInit__socket},
    {"_sha3", PyInit__sha3},
    {"_sre", PyInit__sre},
    {"_ssl", PyInit__ssl},
    {"_stat", PyInit__stat},
    {"_stentry", PyInit__stentry},
    {"_struct", PyInit__struct},
    {"atexit", PyInit_atexit},
    {"binascii", PyInit_binascii},
    {"errno", PyInit_errno},
    {"fcntl", PyInit_fcntl},
    {"grp", PyInit_grp},
    {"math", PyInit_math},
    {"mmap", PyInit_mmap},
    {"posix", PyInit_posix},
    {"pwd", PyInit_pwd},
    {"select", PyInit_select},
    {"syslog", PyInit_syslog},
    {"termios", PyInit_termios},
    {"time", PyInit_time},
    {"zlib", PyInit_zlib},
    {nullptr, nullptr},
};
// clang-format on

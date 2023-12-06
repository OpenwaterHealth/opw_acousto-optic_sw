#include "scan.h"

#define PY_SSIZE_T_CLEAN
#include "system/third_party/python/python-3.7.3/include/python3.7m/Python.h"

static PyObject *ScanError;

static PyObject *start(PyObject *self, PyObject *args) {
  float xLength = -1;
  if (!PyArg_ParseTuple(args, "f", &xLength))
    return NULL;
  return PyLong_FromLong(1);
}

static PyMethodDef ScanMethods[] = {
  { "start",  start, METH_VARARGS, "Start a scan."},
  {NULL, NULL, 0, NULL}        /* Sentinel */
};

static const char ScanDoc[] = "scan module";

static struct PyModuleDef ScanModule = {
  PyModuleDef_HEAD_INIT,
  "scan",   /* name of module */
  ScanDoc,  /* module documentation, may be NULL */
  -1,       /* size of per-interpreter state of the module,
             or -1 if the module keeps state in global variables. */
  ScanMethods
};

PyMODINIT_FUNC PyInit_libscan(void) {
  PyObject *m = PyModule_Create(&ScanModule);
  if (m == NULL)
    return NULL;

  ScanError = PyErr_NewException("scan.error", NULL, NULL);
  Py_INCREF(ScanError);
  PyModule_AddObject(m, "error", ScanError);
  return m;
}

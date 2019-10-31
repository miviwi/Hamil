#include <Python.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <glob.h>
#include <time.h>

//#define DEBUG_OUTPUT

PyObject *eugene_sysv_FindFiles(PyObject *self, PyObject *arg)
{
  if(!PyUnicode_Check(arg)) {
    PyErr_SetString(PyExc_ValueError, "FindFiles() accepts a single 'str' argument!");
    return NULL;
  }

  const char *pattern = PyUnicode_AsUTF8(arg);

  glob_t pglob;
  glob(pattern, 0, NULL, &pglob);

#if defined(DEBUG_OUTPUT)
  printf("glob(%s): gl_pathc=%zu\n", pattern, pglob.gl_pathc);
  for(size_t i = 0; i < pglob.gl_pathc; i++) {
    const char *path = pglob.gl_pathv[i];
    printf("    [%zu] %s\n", i, path);
  }
#endif

  if(!pglob.gl_pathc) {
    PyErr_SetString(PyExc_ValueError, "no files found");
    return NULL;
  }

  PyObject *result = PyList_New(0);
  for(size_t i = 0; i < pglob.gl_pathc; i++) {
    const char *path = pglob.gl_pathv[i];

    struct stat st;
    stat(path, &st);

    time_t last_write = st.st_mtim.tv_sec*10000000 + st.st_mtim.tv_nsec/100;
    off_t size = st.st_size;

    long is_directory = S_ISDIR(st.st_mode);

    PyObject *file_data = PyDict_New();

    PyObject *cFileName       = PyUnicode_FromString(path);
    PyObject *ftLastWriteTime = PyLong_FromUnsignedLongLong(last_write);
    PyObject *nFileSize       = PyLong_FromUnsignedLongLong(size);
    PyObject *bIsDirectory    = PyBool_FromLong(is_directory);

    PyDict_SetItemString(file_data, "cFileName", cFileName);
    PyDict_SetItemString(file_data, "ftLastWriteTime", ftLastWriteTime);
    PyDict_SetItemString(file_data, "nFileSize", nFileSize);
    PyDict_SetItemString(file_data, "bIsDirectory", bIsDirectory);

    PyList_Append(result, file_data);

    Py_XDECREF(file_data);
    Py_XDECREF(cFileName);
    Py_XDECREF(ftLastWriteTime);
    Py_XDECREF(nFileSize);
    Py_XDECREF(bIsDirectory);
  }

  globfree(&pglob);

  return result;
}

char p_local_date[256];
char p_local_time[256];
PyObject *eugene_sysv_GetDateTimeFormat(PyObject *self, PyObject *arg)
{
  if(!PyLong_Check(arg)) {
    PyErr_SetString(PyExc_ValueError, "GetDateTimeFormat() accepts a single 'int' arguemnt!");
    return NULL;
  }

  time_t time_raw  = PyLong_AsUnsignedLongLong(arg);
  time_t time_secs = time_raw / 10000000;    //  'time_raw' is in units of 100ns
                                             // -> convert it to seconds
                                            
  struct tm time_st;
  localtime_r(&time_secs, &time_st);

  PyObject *datetime = PyUnicode_FromFormat(
      "%.2d-%.2d-%d %.2d:%.2d:%.2d",
      time_st.tm_mday, time_st.tm_mon+1 /* [0;11] => [1;12] */, 1900+time_st.tm_year, // Date
      time_st.tm_hour, time_st.tm_min, time_st.tm_sec      // Time
  );

  return datetime;
}

/*
 * List of functions to add to eugene_sysv in exec_eugene_sysv().
 */
static PyMethodDef eugene_sysv_functions[] = {
    {
      "FindFiles",
      (PyCFunction)eugene_sysv_FindFiles, METH_O,
      "win32 FindFirstFile()"
    },

    {
      "GetDateTimeFormat",
      (PyCFunction)eugene_sysv_GetDateTimeFormat, METH_O,
      "win32 Get<Date/Time>Format()"
    },

    { NULL, NULL, 0, NULL } /* marks end of array */
};

/*
 * Initialize eugene_sysv. May be called multiple times, so avoid
 * using static state.
 */
int exec_eugene_sysv(PyObject *module)
{
    PyModule_AddFunctions(module, eugene_sysv_functions);

    PyModule_AddStringConstant(module, "__author__", "bruneron");
    PyModule_AddStringConstant(module, "__version__", "1.0.0");

    return 0; /* success */
}

/*
 * Documentation for eugene_sysv.
 */
PyDoc_STRVAR(eugene_sysv_doc, "SysV interface for Eugene");


static PyModuleDef_Slot eugene_sysv_slots[] = {
    { Py_mod_exec, exec_eugene_sysv },
    { 0, NULL }
};

static PyModuleDef eugene_sysv_def = {
    PyModuleDef_HEAD_INIT,
    "eugene_sysv",
    eugene_sysv_doc,
    0,              /* m_size */
    NULL,           /* m_methods */
    eugene_sysv_slots,
    NULL,           /* m_traverse */
    NULL,           /* m_clear */
    NULL,           /* m_free */
};

PyMODINIT_FUNC PyInit_eugene_sysv() {
    return PyModuleDef_Init(&eugene_sysv_def);
}

#include <python/python.h>
#include <python/object.h>

namespace python {

Object p_globals(nullptr);
PyThreadState *p_main_thread_state = nullptr;

void init()
{
  Py_InitializeEx(0);
  PyEval_InitThreads();

  p_main_thread_state = PyEval_SaveThread();

  p_globals = Py_BuildValue("{s:d}", "a", 123.456);
}

void finalize()
{
  p_globals = nullptr; // Py_DECREF's the underlying PyObject

  PyEval_RestoreThread(p_main_thread_state);
  Py_Finalize();
}

std::string eval(const char *input)
{
  Object co = Py_CompileString(input, "$", Py_eval_input);
  if(!co) return "";

  Object result = PyEval_EvalCode(*co, *p_globals, *p_globals);

  return result.repr();
}

Interpreter::Interpreter()
{
  PyEval_AcquireLock();
  m_thread_state = Py_NewInterpreter();

  unlock();
}

Interpreter::~Interpreter()
{
  lock();
  Py_EndInterpreter(m_thread_state);
  PyEval_ReleaseLock();
}

void Interpreter::lock()
{
  PyEval_AcquireThread(m_thread_state);
}

void Interpreter::unlock()
{
  PyEval_ReleaseThread(m_thread_state);
}

Exception Exception::fetch()
{
  return { "", "", "" };
}

Exception::Exception(std::string type, std::string value, std::string traceback) :
  m_type(type), m_value(value), m_traceback(traceback)
{
}

}
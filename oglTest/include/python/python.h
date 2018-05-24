#pragma once

#include <common.h>

#include <Python.h>

#include <string>

namespace python {

void init();
void finalize();

std::string eval(const char *input);

class Interpreter {
public:
  Interpreter();
  ~Interpreter();

  void lock();
  void unlock();

private:
  PyThreadState *m_thread_state;
};

class Exception {
public:
  static Exception fetch();

protected:
  Exception(std::string type, std::string value, std::string traceback);

private:
  std::string m_type, m_value, m_traceback;
};

}
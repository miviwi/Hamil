#pragma once

#include <common.h>

#include <Python.h>

#include <string>

namespace python {

void init();
void finalize();

std::string eval(const char *input);

class Exception {
public:
  static Exception fetch();

  static bool occured();

  const std::string& type() const;
  const std::string& value() const;
  const std::string& traceback() const;

protected:
  Exception(std::string&& type, std::string&& value, std::string&& traceback);

private:
  std::string m_type, m_val, m_trace;
};

}
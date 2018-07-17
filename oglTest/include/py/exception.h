#pragma once

#include <common.h>
#include <py/object.h>

#include <string>

namespace py {

class Exception {
public:
  static Exception fetch();

  static bool occured();

  const Object& type() const;
  const Object& value() const;
  const Object& traceback() const;

protected:
  Exception(Object type, Object value, Object traceback);

private:
  Object m_type, m_val, m_trace;
};

}

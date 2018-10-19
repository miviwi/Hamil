#pragma once

#include <common.h>
#include <py/object.h>
#include <py/types.h>

#include <string>

namespace py {

class Exception {
public:
  static Exception fetch();

  static bool occured();

  const Type& type() const;
  const Object& value() const;
  const Object& traceback() const;

protected:
  Exception(Type type, Object value, Object traceback);

private:
  Type m_type;
  Object m_val, m_trace;
};

}

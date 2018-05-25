#pragma once

#include <python/python.h>
#include <python/object.h>

namespace python {

class Module : public Object {
public:
  Module(const char *name);
};

}

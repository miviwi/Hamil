#include <python/module.h>

namespace python {

Module::Module(const char *name) :
  Object(PyImport_AddModule(name))
{
}

}
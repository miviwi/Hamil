#include <cli/sh.h>

#include <py/python.h>

namespace cli {

int shell(int argc, char *argv[])
{
  return py::main(argc, argv);
}

}
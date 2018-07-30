#include <cli/cli.h>

#include <util/opts.h>

#include <cstdio>

namespace cli {

int args(int argc, char *argv[])
{
  if(argc == 1) return 0;

  auto opts = util::ConsoleOpts()
    .boolean("resource-gen", "generate *.meta files (resource descriptors) from images, sound files etc.")
    .list("resources", "list of resources for resource-gen")
    ;

  puts(opts.doc().data());
  opts.parse(argc, argv);

  opts.dbg_PrintOpts();

  if(opts("resource-gen")) {
    try {
      if(auto resources = opts("resources")) {
        resourcegen(resources->list());
      } else {
        resourcegen({});
      }
    } catch(const GenError& e) {
      printf("error: %s\n", e.what.data());
      return -1;
    }

    return 1;
  }

  return 0;
}

}
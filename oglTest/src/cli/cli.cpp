#include <cli/cli.h>
#include <cli/sh.h>
#include <cli/resourcegen.h>

#include <util/opts.h>

#include <cstdio>

namespace cli {

int args(int argc, char *argv[])
{
  auto opts = util::ConsoleOpts()
    .boolean("sh", "launch a headless interactive scripting shell")

    .boolean("resource-gen", "generate *.meta files (resource descriptors) from images, sound files etc.")
    .list("resources", "list of resources for resource-gen")
    ;

  // Print usage if options are invalid or weren't given
  if(argc == 1 || opts.parse(argc, argv)) {
    puts(opts.doc().data());
    return 0;
  }
  
  if(opts("sh")->b()) {
     shell(argc-1, argv+1);

     return 1;
  } else if(opts("resource-gen")->b()) {
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
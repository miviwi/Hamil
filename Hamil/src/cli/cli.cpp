#include <cli/cli.h>
#include <cli/sh.h>
#include <cli/resourcegen.h>

#include <util/opts.h>

#include <cstdio>

namespace cli {

int args(int argc, char *argv[])
{
  auto opts = util::ConsoleOpts()
    .boolean("help", "prints the following descriptions")

    .boolean("sh", "launch a headless interactive scripting shell")

    .boolean("resource-gen", "generate *.meta files (resource descriptors) from images, sound files etc.")
    .list("resources", "list of resource files for resource-gen")
    .list("types", "list of resource file types to be processed by resource-gen")
    ;

  int exit_code = 0;

  // Print usage if options are invalid or weren't given
  if(argc == 1 || opts.parse(argc, argv)) {
    puts(opts.doc().data());
  } else {          // The options were parsed successfully - set
    exit_code = 1;  //   the exit_code to 1 which indicates success
  }

  if(opts("help")->b()) {
    puts(opts.doc().data());
  } else if(opts("sh")->b()) {
    shell(argc-1, argv+1);
  } else if(opts("resource-gen")->b()) {
    try {
      std::vector<std::string> resources;
      std::set<std::string> types;

      if(auto opt = opts("resources")) resources = opt->list();
      if(auto opt = opts("types")) types.insert(opt->list().cbegin(), opt->list().cend());

      resourcegen(resources, types);
    } catch(const GenError& e) {
      printf("error: %s\n", e.what.data());

      return -1;
    }

    exit_code = 1;
  }

  return exit_code;
}

}
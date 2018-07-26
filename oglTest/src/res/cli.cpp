#include <res/cli.h>
#include <res/res.h>

#include <win32/file.h>

#include <functional>
#include <regex>

namespace res {

static const std::regex p_name_regex("^((?:[^/ ]+/)*)([^./ ]*)\\.([a-z]+)$", std::regex::optimize);
void resourcegen(std::vector<std::string> resources)
{
  init();

  if(resources.empty()) {
    std::function<void(const std::string&)> enum_resources = [&](const std::string& path) {
      win32::FileQuery q((path + "*").data());

      q.foreach([&](const char *name, win32::FileQuery::Attributes attrs) {
        auto full_name = path + name;

        if(attrs & win32::FileQuery::IsDirectory) {
          enum_resources((full_name + "/").data());
        } else {
          resources.emplace_back(full_name);
        }
      });
    };

    enum_resources("");
  }

  for(const auto& resource : resources) {
    // [1] = path
    // [2] = name
    // [3] = extension
    std::smatch matches;
    if(!std::regex_match(resource, matches, p_name_regex)) continue;

    auto path   = matches[1].str(),
      name      = matches[2].str(),
      extension = matches[3].str();

    if(!path.empty()) path.pop_back();
  }
}


}
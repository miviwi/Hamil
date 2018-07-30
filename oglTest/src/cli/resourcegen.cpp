#include <cli/cli.h>

#include <yaml/document.h>
#include <yaml/node.h>
#include <win32/file.h>
#include <util/str.h>
#include <util/format.h>

#include <res/res.h>
#include <res/manager.h>
#include <res/resource.h>
#include <res/text.h>
#include <res/shader.h>

#include <string>
#include <vector>
#include <map>
#include <functional>
#include <regex>

namespace cli {

static yaml::Document shadergen(win32::File& file,
  const std::string& name, const std::string& path, const std::string& extension);

using GenFunc = std::function<yaml::Document(win32::File& file,
  const std::string& name, const std::string& path, const std::string& extension)>;
static const std::map<std::string, GenFunc> p_gen_fns = {
  // GPU program (multiple shaders)
  { "glsl", shadergen },

  // Vertex Shader
  { "vert", shadergen },

  // Geometry Shader
  { "geom", shadergen },

  // Fragment Shader
  { "frag", shadergen },
};

static const std::regex p_name_regex("^((?:[^/ ]+/)*)([^./ ]*)\\.([a-z]+)$", std::regex::optimize);
void resourcegen(std::vector<std::string> resources)
{
  res::init();

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

    if(!path.empty()) path.pop_back(); // Strip the leading '/'

    auto it = p_gen_fns.find(extension);
    if(it == p_gen_fns.end()) continue; // No handler for this file type

    win32::File f(resource.data(), win32::File::Read, win32::File::OpenExisting);

    auto meta = it->second(f, name, path, extension);
    auto meta_data = meta.toString();

    auto f_name = util::fmt("./%s%s.meta",
      meta("path")->as<yaml::Scalar>()->str(),
      meta("name")->as<yaml::Scalar>()->str());

    win32::File f_meta(f_name.data(), win32::File::Write, win32::File::CreateAlways);

    // If there's more then ULONG_MAX bytes of data - oh well
    f_meta.write(meta_data.data(), (ulong)meta_data.size());
  }
}

enum PragmaType {
  PragmaInvalid,

  PragmaShader,     // Marks the start of a Vertex/Geometry/Fragment shader
                    // in a file
                    //   #pragma shader(<Vertex, Geometry, Fragment>)

  PragmaImport,     // #pragma import(shader.frag)

  PragmaExport,     // Must be the first non-empty line in the file!
                    //   #pragma export(myfunc.frag)

};

struct PragmaInfo {
  std::regex r;
  PragmaType type;
};

static const std::vector<PragmaInfo> p_pragmas = {
  { std::regex("^#pragma\\s+shader\\s*\\(\\s*(?:(vertex)|(geometry)|(fragment))\\s*\\)$",
    std::regex::optimize), PragmaShader },

  { std::regex("^#pragma\\s+import\\s*\\(\\s*([^ ]+)\\s*\\)$", std::regex::optimize), PragmaImport },
  { std::regex("^#pragma\\s+export\\s*\\(\\s*([^ ]+)\\s*\\)$", std::regex::optimize), PragmaExport },

};

static yaml::Document shadergen(win32::File& file,
  const std::string& name, const std::string& path, const std::string& extension)
{
  auto view = file.map(win32::File::ProtectRead);
  auto source = view.get<const char>();

  auto guid = res::resource().guid<res::Shader>(name, path);

  // count non-preprocessor lines
  size_t line_no = 0;

  yaml::Node::Ptr pvert, pgeom, pfrag;

  auto meta = new yaml::Mapping;
  meta->append(yaml::Scalar::from_str("tag"),  yaml::Scalar::from_str("shader"));
  meta->append(yaml::Scalar::from_str("guid"), yaml::Scalar::from_str(util::fmt("0x%.16llx", guid)));
  meta->append(yaml::Scalar::from_str("name"), yaml::Scalar::from_str(name));
  meta->append(yaml::Scalar::from_str("path"), yaml::Scalar::from_str(path.empty() ? "" : "/" + path));

  yaml::Sequence *section = nullptr;

  auto init_section = [&](yaml::Node::Ptr& p) -> bool
  {
    if(p) return false;

    section = new yaml::Sequence;
    p = yaml::Node::Ptr(section);

    return true;
  };

  if(extension == "vert") {
    init_section(pvert);
  } else if(extension == "geom") {
    init_section(pgeom);
  } else if(extension == "frag") {
    init_section(pfrag);
  }

  util::splitlines(source, [&](const std::string& line) {
    auto stripped = util::strip(line);
    if(stripped.empty()) return;

    if(stripped.front() != '#') {
      // a non-preprocessor line
      line_no++;
      return;
    }

    puts(stripped.data());

    for(const auto& p : p_pragmas) {
      std::smatch matches;
      if(!std::regex_match(stripped, matches, p.r)) continue;

      switch(p.type) {
      case PragmaShader:
        puts("    Shader");
        if(matches[1].matched /* vertex */) {
          if(!init_section(pvert)) throw GenError("shader(vertex) specified twice!");
        } else if(matches[2].matched /* geometry */) {
          if(!init_section(pgeom)) throw GenError("shader(geometry) specified twice!");
        } else if(matches[3].matched /* fragment */) {
          if(!init_section(pfrag)) throw GenError("shader(fragment) specified twice!");
        }
        break;

      case PragmaImport:
        printf("    Import(%s)\n", matches[1].str().data());
        if(!section) break;

        section->append(yaml::Node::Ptr(
          new yaml::Scalar(matches[1].str(), yaml::Node::Tag(res::Shader::LibSource.get()))
        ));
        break;

      case PragmaExport:
        printf("    Export(%s)\n", matches[1].str().data());
        break;

      default: assert(0); // unreachable
      }

    }

  });

  if(pvert) meta->append(yaml::Scalar::from_str("vertex"),   pvert);
  if(pgeom) meta->append(yaml::Scalar::from_str("geometry"), pgeom);
  if(pfrag) meta->append(yaml::Scalar::from_str("fragment"), pfrag);

  auto doc = yaml::Document(yaml::Node::Ptr(meta));
  puts(doc.toString().data());

  return doc;
}

}
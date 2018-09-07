#include <cli/resourcegen.h>

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
#include <res/image.h>

#include <string>
#include <vector>
#include <map>
#include <functional>
#include <regex>

#include <stb_image/stb_image.h>

namespace cli {

static yaml::Document shadergen(win32::File& file,
  const std::string& name, const std::string& path, const std::string& extension);

static yaml::Document imagegen(win32::File& file,
  const std::string& name, const std::string& path, const std::string& extension);

using GenFunc = std::function<
  yaml::Document(win32::File& file,
    const std::string& name, const std::string& path, const std::string& extension)
  >;
static const std::map<std::string, GenFunc> p_gen_fns = {
  // ------------------ Shaders -----------------------------
  { "glsl", shadergen }, // GPU program (multiple shaders)
  
  { "vert", shadergen }, // Vertex Shader   (can contain other shaders!)
  { "geom", shadergen }, // Geometry Shader        ---- || ----
  { "frag", shadergen }, // Fragment Shader        ---- || ----

  // ------------------ Images  -----------------------------
  { "jpg",  imagegen },  // JPEG Image
  { "jpeg", imagegen },  // -- || --

  { "png",  imagegen },  // PNG Image

  { "bmp",  imagegen },  // BMP Image

  { "tif",  imagegen },  // TIFF Image
  { "tiff", imagegen },  // -- || -- 
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

    auto f_name = util::fmt("./%s/%s.meta",
      meta("path")->as<yaml::Scalar>()->str(),
      meta("name")->as<yaml::Scalar>()->str());

    win32::File f_meta(f_name.data(), win32::File::Write, win32::File::CreateAlways);

    // If there's more than ULONG_MAX bytes of data - oh well
    f_meta.write(meta_data.data(), (ulong)meta_data.size());
  }
}

template <typename T>
yaml::Mapping *make_meta(const std::string& name, const std::string& path)
{
  auto full_path = path.empty() ? "" : "/" + path;
  auto guid = res::resource().guid<T>(name, full_path);

  return yaml::ordered_string_mapping({
    { "guid", util::fmt("0x%.16llx", guid) },
    { "tag",  T::tag().get() },
    { "name", name },
    { "path", full_path },
  });
}

enum PragmaType {
  PragmaInvalid,

  PragmaShader,     // Marks the start of a Vertex/Geometry/Fragment section
                    // in a file
                    //   #pragma shader(<Vertex, Geometry, Fragment>)

  PragmaImport,     //   #pragma import(shader.frag)

  PragmaExport,     // Exports the current section
                    //   #pragma export()
};

struct PragmaInfo {
  PragmaType type;
  std::regex r;

  PragmaInfo(PragmaType type_, const std::string& name, const std::string& args_regex) :
    type(type_),
    r("^#pragma\\s+" + name + "\\s*\\(\\s*" + args_regex + "\\s*\\)$", std::regex::optimize)
  { }
};

static const std::vector<PragmaInfo> p_pragmas = {
  { PragmaShader, "shader", "(?:(vertex)|(geometry)|(fragment))" },

  { PragmaImport, "import", "([^ ]+)" },
  { PragmaExport, "export", ""        },
};

static yaml::Document shadergen(win32::File& file,
  const std::string& name, const std::string& path, const std::string& extension)
{
  auto view = file.map(win32::File::ProtectRead);
  auto source = view.get<const char>();

  // count non-preprocessor lines
  size_t line_no = 0;

  yaml::Node::Ptr pvert, pgeom, pfrag;

  auto meta = make_meta<res::Shader>(name, path);

  yaml::Sequence *section = nullptr;
  std::string inline_source;

  auto append_source = [&](std::string src, auto tag) {
    if(tag == res::Shader::InlineSource) {
      section->append(yaml::Node::Ptr(
        new yaml::Scalar(src, tag.get(), yaml::Node::Literal)
      ));
    } else {
      section->append(yaml::Node::Ptr(
        new yaml::Scalar(src, tag.get())
      ));
    }
  };

  auto init_section = [&](yaml::Node::Ptr& p) -> bool {
    if(p) return false;

    if(section) {
      append_source(inline_source, res::Shader::InlineSource);
      inline_source.clear();
    }

    section = new yaml::Sequence({}, yaml::Node::Block);
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

  auto error_message = [&](const char *message) -> std::string {
    return util::fmt("%s/%s.%s: %s", path.data(), name.data(), extension.data(), message);
  };
  
  auto error_specified_twice = [&](const char *stage) -> std::string {
    return error_message(util::fmt("shader(%s) specified twice!").data());
  };

  util::splitlines(source, [&](const std::string& line) {
    auto stripped = util::strip(line);

    if(stripped.empty() || stripped.front() != '#') { // a non-preprocessor line
      inline_source += line;

      line_no++;
      return;
    }

    for(const auto& p : p_pragmas) {
      std::smatch matches;
      if(!std::regex_match(stripped, matches, p.r)) continue;

      switch(p.type) {
      case PragmaShader:
        if(matches[1].matched /* vertex */) {
          if(!init_section(pvert)) throw GenError(error_specified_twice("vertex"));
        } else if(matches[2].matched /* geometry */) {
          if(!init_section(pgeom)) throw GenError(error_specified_twice("geometry"));
        } else if(matches[3].matched /* fragment */) {
          if(!init_section(pfrag)) throw GenError(error_specified_twice("fragment"));
        }
        break;

      case PragmaImport:
        if(!section) throw GenError(error_message("importing into undefined section!"));

        append_source(inline_source, res::Shader::InlineSource);
        inline_source.clear();

        append_source(matches[1].str(), res::Shader::ImportSource);
        return; // Prevent import #pragma's from being added to the source

      case PragmaExport:
        if(!section) throw GenError(error_message("exporting undefined section!"));

        section->tag(yaml::Node::Tag(res::Shader::ExportSource.get()));
        break;

      default: assert(0); // unreachable
      }
    }

    inline_source += line; // add non-import preprocessor lines
  });

  // Append left-over lines
  if(section) append_source(inline_source, res::Shader::InlineSource);

  if(pvert) meta->append(yaml::Scalar::from_str("vertex"),   pvert);
  if(pgeom) meta->append(yaml::Scalar::from_str("geometry"), pgeom);
  if(pfrag) meta->append(yaml::Scalar::from_str("fragment"), pfrag);

  return yaml::Document(yaml::Node::Ptr(meta));
}

static yaml::Document imagegen(win32::File& file,
  const std::string& name, const std::string& path, const std::string& extension)
{
  auto location = util::fmt(".%s/%s.%s", path.data(), name.data(), extension.data());
  std::optional<yaml::Document> params = std::nullopt;

  printf("processing image: .%s...\n", location.data());

  try {
    auto fname = util::fmt(".%s/%s.imageparams", path.data(), name.data());
    win32::File f_params(fname.data(), win32::File::Read, win32::File::OpenExisting);

    auto f_params_view = f_params.map(win32::File::ProtectRead);

    params = yaml::Document::from_string(f_params_view.get<const char>(), f_params.size());
  } catch(const win32::File::Error&) {
    // No additional params...
  }

  if(params) {
    printf("    found params:\n\n%s\n", params->toString().data());
  } else {
    printf("    no params\n");
  }

  int width, height, channels;

  auto image_view = file.map(win32::File::ProtectRead);
  auto image      = stbi_load_from_memory(image_view.get<byte>(), (int)file.size(), &width, &height, &channels, 0);

  if(!image) throw GenError(util::fmt("%s: invalid image file!", location.data()));

  auto meta = make_meta<res::Image>(name, path)->append(
    yaml::Scalar::from_str("location"),
    yaml::Node::Ptr(new yaml::Scalar(location, yaml::Node::Tag("!file")))
  )->append(
    yaml::Scalar::from_str("dimensions"),
    yaml::Node::Ptr(yaml::isequence({ width, height }))
  );

  if(params) meta->concat(params->get());

  printf("        ...done!\n\n");

  auto doc = yaml::Document(yaml::Node::Ptr(meta));
  puts(doc.toString().data());


  return doc;
}

}
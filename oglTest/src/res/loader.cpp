#include <res/loader.h>
#include <res/manager.h>
#include <res/resource.h>
#include <res/text.h>
#include <res/shader.h>

#include <util/format.h>
#include <win32/file.h>
#include <win32/panic.h>
#include <yaml/document.h>
#include <yaml/node.h>
#include <yaml/schema.h>

#include <cassert>
#include <cstdio>
#include <cstring>
#include <vector>
#include <unordered_map>
#include <functional>

namespace res {

SimpleFsLoader::SimpleFsLoader(const char *base_path) :
  m_path(base_path)
{
  win32::current_working_directory(base_path);
  
  enumAvailable("./"); // enum all resources starting from 'base_path'
}

static yaml::Schema p_meta_generic_schema = 
  yaml::Schema()
    .scalar("guid", yaml::Scalar::Int)
    .scalar("tag",  yaml::Scalar::String)
    .file("name")
    .path("path")
  ;

static const std::unordered_map<Resource::Tag, yaml::Schema> p_meta_schemas = {
  { Text::tag(),   yaml::Schema()
                             .scalar("location", yaml::Scalar::Tagged) },
  { Shader::tag(), yaml::Schema()
                             .scalarSequence("vertex",   yaml::Scalar::Tagged)
                             .scalarSequence("geometry", yaml::Scalar::Tagged, yaml::Optional)
                             .scalarSequence("fragment", yaml::Scalar::Tagged, yaml::Optional) },

};

static const std::unordered_map<Resource::Tag, SimpleFsLoader::LoaderFn> p_loader_fns = {
  { Text::tag(),   &SimpleFsLoader::loadText   },
  { Shader::tag(), &SimpleFsLoader::loadShader },
};

Resource::Ptr SimpleFsLoader::load(Resource::Id id, LoadFlags flags)
{
  auto it = m_available.find(id);
  if(it == m_available.end()) return Resource::Ptr();  // Resource not found!

  // The file was already parsed (in enumOne()) so we can assume it is valid
  auto meta = yaml::Document::from_string(it->second.data(), it->second.size()-1 /* libyaml doesn't like '\0' */);
  if(p_meta_generic_schema.validate(meta)) throw InvalidResourceError(id);

  puts(meta.toString().data());

  auto tag = ResourceManager::make_tag(meta("tag")->as<yaml::Scalar>()->str());
  if(!tag) throw InvalidResourceError(id);

  auto schema_it = p_meta_schemas.find(tag.value());
  assert(schema_it != p_meta_schemas.end() && "schema missing in p_meta_schemas!");

  if(schema_it->second.validate(meta)) throw InvalidResourceError(id);

  auto loader_it = p_loader_fns.find(tag.value());
  assert(loader_it != p_loader_fns.end() && "loading function missing in SimpleFsLoader!");

  auto loader = loader_it->second;

  return (this->*loader)(id, meta);
}

void SimpleFsLoader::enumAvailable(std::string path)
{
  using win32::FileQuery;

  // Snoop subdirectories first
  std::string dir_query_str = path + "*";
  win32::FileQuery dir_query(dir_query_str.data());

  dir_query.foreach([this,&path](const char *name, FileQuery::Attributes attrs) {
    // Have to manually check whether 'name' is a directory
    bool is_directory = attrs & FileQuery::IsDirectory;
    if(!is_directory) return;

    // 'name' is a directory - descend into it
    enumAvailable(path + name + "/");
  });

  // Now query *.meta files (resource descriptors)
  std::string meta_query_str = path + "*.meta";
  win32::FileQuery meta_query;
  try {
    meta_query = win32::FileQuery(meta_query_str.data());
  } catch(const win32::FileQuery::Error&) {
    return; // no .meta files in current directory
  }

  meta_query.foreach([this,path](const char *name, FileQuery::Attributes attrs) {
    // if there's somehow a directory that fits *.meta ignore it
    if(attrs & FileQuery::IsDirectory) return;

    auto full_path = path + name;

    try {
      // Because we are in the middle of initializing the ResourceManager
      //   the low-level win32::File interface has to be used which means
      //   synchronous IO.
      // Maybe res::init() could be run on a separate thread so events
      //   could still be processed?
      // Regardless, a splash screen should be put up on the screen to let the
      //   user know the application is doing something and isn't just frozen
      win32::File file(full_path.data(), win32::File::Read, win32::File::OpenExisting);

      std::vector<char> file_buf(file.size()+1); // put '\0' at the end
      file.read(file_buf.data());

      auto id = enumOne(file_buf.data(), file.size(), file.fullPath());
      if(id == Resource::InvalidId) return;

      printf("resource %s: 0x%.16llx\n", name, id);

      m_available.emplace(
        id,
        std::move(file_buf)
      );
    } catch(const win32::File::Error&) {
      // panic, there really shouldn't be an exception here
      win32::panic(util::fmt("error opening file \"%s\"", full_path.data()).data(), win32::FileOpenError);
    }
  });
}

Resource::Id SimpleFsLoader::enumOne(const char *meta_file, size_t sz, const char *full_path)
{
  yaml::Document meta;
  try {
    meta = yaml::Document::from_string(meta_file, sz);
  } catch(const yaml::Document::Error& e) {
    printf("warning: meta file '%s' could not be parsed\n", full_path);
    printf("         %s\n", e.what().data());
    return Resource::InvalidId;
  }

  auto guid_node = meta("guid");
  if(guid_node && guid_node->type() == yaml::Node::Scalar) {
    auto guid = guid_node->as<yaml::Scalar>();

    // make sure the node is a Scalar::Int
    if(guid->dataType() == yaml::Scalar::Int) return guid->ui();
  }

  printf("warning: meta file '%s' doesn't have a 'guid' node (or it's not a Scalar)\n", full_path);
  return Resource::InvalidId;
}

Resource::Ptr SimpleFsLoader::loadText(Resource::Id id, const yaml::Document& meta)
{
  // execution only reaches here when 'meta' has passed validation
  // so we can assume it's valid
  auto location = meta("location")->as<yaml::Scalar>();
  auto name     = meta("name")->as<yaml::Scalar>();
  auto path     = meta("path")->as<yaml::Scalar>();

  win32::File f(location->str(), win32::File::Read, win32::File::OpenExisting);
  win32::FileView view = f.map(win32::File::ProtectRead);

  return Text::from_file(view.get<char>(), f.size(), id, name->str(), path->str());
}

Resource::Ptr SimpleFsLoader::loadShader(Resource::Id id, const yaml::Document& meta)
{
  auto name = meta("name")->as<yaml::Scalar>();
  auto path = meta("path")->as<yaml::Scalar>();

  return Shader::from_yaml(meta, id, name->str(), path->str());
}

}
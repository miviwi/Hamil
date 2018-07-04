#include <res/loader.h>
#include <res/manager.h>
#include <res/resource.h>
#include <res/text.h>

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

static yaml::Schema p_meta_schema = 
  yaml::Schema()
    .scalar("guid",     yaml::Scalar::Int)
    .scalar("tag",      yaml::Scalar::String)
    .scalar("location", yaml::Scalar::Tagged)
  ;

static const std::unordered_map<Resource::Tag, SimpleFsLoader::LoaderFn, Resource::Tag::Hash> p_loader_fns = {
  { TextResource::tag(), &SimpleFsLoader::loadText },
};

Resource::Ptr SimpleFsLoader::load(Resource::Id id, LoadFlags flags)
{
  auto it = m_available.find(id);
  if(it == m_available.end()) return Resource::Ptr();  // Resource not found!

  // The file was already parsed (in enumOne()) so we can assume it is valid
  auto meta = yaml::Document::from_string(it->second.data(), it->second.size()-1 /* libyaml doesn't like '\0' */);

  puts(meta.toString().data());

  if(p_meta_schema.validate(meta)) throw InvalidResourceError(id);

  auto tag      = meta("tag")->as<yaml::Scalar>();
  auto location = meta("location")->as<yaml::Scalar>();

  printf("resource(0x%.16llx): %s %s(%s)\n", id,
    tag->str(), location->tag().value().data(), location->str());

  LoaderFn loader = nullptr;
  if(auto t = ResourceManager::make_tag(tag->str())) {
    auto it = p_loader_fns.find(t.value());
    assert(it != p_loader_fns.end() && "loading function missing in SimpleFsLoader!");

    loader = it->second;
  } else {
    throw InvalidResourceError(id);
  }

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

  meta_query.foreach([this](const char *name, FileQuery::Attributes attrs) {
    // if there's somehow a directory that fits *.meta ignore it
    if(attrs & FileQuery::IsDirectory) return;

    try {
      // Because we are in the middle of initializing the ResourceManager
      //   the low-level win32::File interface has to be used which means
      //   synchronous IO.
      // Maybe res::init() could be run on a separate thread so events
      //   could still be processed?
      // Regardless, a splash screen should be put up on the screen to let the
      //   user know the application is doing something and isn't just frozen
      win32::File file(name, win32::File::Read, win32::File::OpenExisting);

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
      win32::panic(util::fmt("error opening file \"%s\"", name).data(), -10);
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

  win32::File f(location->str(), win32::File::Read, win32::File::OpenExisting);
  win32::FileView view = f.map(win32::File::ProtectRead);

  return TextResource::from_file(view.get<char>(), f.size(), id, "", location->str());
}

}
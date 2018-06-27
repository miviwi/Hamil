#include <res/loader.h>

#include <util/format.h>
#include <win32/file.h>
#include <win32/panic.h>
#include <yaml/document.h>
#include <yaml/node.h>

#include <cstdio>
#include <cstring>
#include <vector>

namespace res {

SimpleFsLoader::SimpleFsLoader(const char *base_path)
{
  win32::current_working_directory(base_path);
  
  enumAvailable("./"); // enum all resources starting from 'base_path'
}

Resource::Ptr SimpleFsLoader::load(Resource::Tag tag, Resource::Id id, LoadFlags flags)
{
  return Resource::Ptr();
}

void SimpleFsLoader::enumAvailable(std::string path)
{
  using win32::FileQuery;

  // Snoop subfolders first
  std::string dir_query_str = path + "*";
  win32::FileQuery dir_query(dir_query_str.c_str());

  dir_query.foreach([this,&path](const char *name, FileQuery::Attributes attrs) {
    // Have to manually check whether 'name' is a directory
    bool is_directory = attrs & FileQuery::IsDirectory;
    if(!is_directory) return;

    // 'name' is a directory - descend into it
    enumAvailable(path + name + "/");
  });

  // Now query *.meta files (asset descriptors)
  std::string meta_query_str = path + "*.meta";
  win32::FileQuery meta_query;
  try {
    meta_query = win32::FileQuery(meta_query_str.c_str());
  } catch(const win32::FileQuery::Error&) {
    return; // no .meta files in current directory
  }

  meta_query.foreach([this](const char *name, FileQuery::Attributes attrs) {
    // if there's somehow a directory that fits *.meta ignore it
    if(attrs & FileQuery::IsDirectory) return;

    try {
      // Because we are in the middle of initializing the ResourceManager
      //   the low-level win32::File interface has to be used which means
      //   synchronous IO, maybe res::init() could be run on a separate
      //   thread so events could still be processed?
      // Regardless, a splash screen should be put up on the screen to let the
      //   user know the application is doing something and isn't just frozen
      win32::File file(name, win32::File::Read, win32::File::OpenExisting);

      std::vector<char> file_buf(file.size()+1); // put '\0' at the end
      file.read(file_buf.data());

      auto id = enumOne(file_buf.data(), file.size(), file.fullPath());
      if(id == Resource::InvalidId) return;

      printf("%s: %.16llx\n", name, id);

      m_available.insert({
        id,
        file.fullPath()
      });
    } catch(const win32::File::Error&) {
      // panic, there really shouldn't be an exception here
      win32::panic(util::fmt("error opening file \"%s\"", name).c_str(), -10);
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
    printf("         %s\n", e.what().c_str());
    return Resource::InvalidId;
  }

  // query for 'guid'
  auto guid_node = meta("guid");
  if(guid_node && guid_node->type() == yaml::Node::Scalar) {
    auto guid = guid_node->as<yaml::Scalar>();

    // make sure the node is a Scalar::Int
    if(guid->dataType() == yaml::Scalar::Int) return guid->ui();
  }

  printf("warning: meta file '%s' doesn't have a 'guid' node (or it's not a Scalar)\n", full_path);
  return Resource::InvalidId;
}

}
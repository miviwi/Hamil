#include <res/loader.h>
#include <res/manager.h>
#include <res/resource.h>
#include <res/text.h>
#include <res/shader.h>
#include <res/image.h>
#include <res/mesh.h>

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
#include <map>
#include <unordered_map>
#include <functional>

namespace res {

ResourceManager& ResourceLoader::manager()
{
  return *m_man;
}

ResourceLoader *ResourceLoader::init(ResourceManager *manager)
{
  m_man = manager;

  return this;
}

Resource::Ptr ResourceLoader::load(Resource::Id id, LoadFlags flags)
{
  assert(m_man && "load() called before init()!");

  return doLoad(id, flags);
}

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
                             .scalarSequence("vertex",   yaml::Scalar::Tagged, yaml::Optional)
                             .scalarSequence("geometry", yaml::Scalar::Tagged, yaml::Optional)
                             .scalarSequence("fragment", yaml::Scalar::Tagged, yaml::Optional) },
  { Image::tag(),  yaml::Schema()
                             .scalar("location", yaml::Scalar::Tagged)
                             .scalar("channels", yaml::Scalar::String, yaml::Optional)
                             .scalar("flip_vertical", yaml::Scalar::Boolean, yaml::Optional)
                             .scalarSequence("dimensions", yaml::Scalar::Int) },
  { Mesh::tag(),   yaml::Schema()
                             .scalar("location", yaml::Scalar::Tagged)
                             .mapping("vertex")
                             .scalar("indexed", yaml::Scalar::Boolean)
                             .scalar("primitive", yaml::Scalar::String) },
};

static const std::unordered_map<Resource::Tag, SimpleFsLoader::LoaderFn> p_loader_fns = {
  { Text::tag(),   &SimpleFsLoader::loadText   },
  { Shader::tag(), &SimpleFsLoader::loadShader },
  { Image::tag(),  &SimpleFsLoader::loadImage  },
  { Mesh::tag(),   &SimpleFsLoader::loadMesh   },
};

Resource::Ptr SimpleFsLoader::doLoad(Resource::Id id, LoadFlags flags)
{
  auto it = m_available.find(id);
  if(it == m_available.end()) return Resource::Ptr();  // Resource not found!

  // The file was already parsed (in enumOne()) so we can assume it is valid
  auto meta = yaml::Document::from_string(it->second.data(), it->second.size()-1 /* libyaml doesn't like '\0' */);
  if(p_meta_generic_schema.validate(meta)) throw InvalidResourceError(id);

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

      printf("found resource %-25s: 0x%.16llx\n", full_path.data(), id);

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

SimpleFsLoader::NamePathTuple SimpleFsLoader::name_path(const yaml::Document& meta)
{
  return std::make_tuple(
    meta("name")->as<yaml::Scalar>(),
    meta("path")->as<yaml::Scalar>()
  );
}

SimpleFsLoader::NamePathLocationTuple SimpleFsLoader::name_path_location(const yaml::Document& meta)
{
  return std::make_tuple(
    meta("name")->as<yaml::Scalar>(),
    meta("path")->as<yaml::Scalar>(),
    meta("location")->as<yaml::Scalar>()
  );
}

Resource::Ptr SimpleFsLoader::loadText(Resource::Id id, const yaml::Document& meta)
{
  // execution only reaches here when 'meta' has passed validation
  // so we can assume it's valid
  const yaml::Scalar *name, *path, *location;
  std::tie(name, path, location) = name_path_location(meta);

  auto req = IORequest::read_file(location->str());
  manager().requestIo(req);

  auto text = manager().waitIo(req);

  return Text::from_file(text.get<const char>(), text.size(), id, name->str(), path->str());
}

Resource::Ptr SimpleFsLoader::loadShader(Resource::Id id, const yaml::Document& meta)
{
  const yaml::Scalar *name, *path;
  std::tie(name, path) = name_path(meta);

  return Shader::from_yaml(meta, id, name->str(), path->str());
}

static const std::map<std::string, unsigned> p_num_channels = {
  { "i",    1 },
  { "ia",   2 },
  { "rgb",  3 },
  { "rgba", 4 },
};

Resource::Ptr SimpleFsLoader::loadImage(Resource::Id id, const yaml::Document& meta)
{
  const yaml::Scalar *name, *path, *location;
  std::tie(name, path, location) = name_path_location(meta);

  auto channels_node = meta("channels");
  auto dims          = meta("dimensions");
  auto flip_vertical = meta("flip_vertical");

  unsigned num_channels = 0;
  if(channels_node) {
    auto channels = channels_node->as<yaml::Scalar>()->str();
    auto it = p_num_channels.find(channels);
    if(it == p_num_channels.end()) throw InvalidResourceError(id);

    num_channels = it->second;
  }

  auto width  = (int)dims->get<yaml::Scalar>(0)->i();
  auto height = (int)dims->get<yaml::Scalar>(1)->i();

  unsigned flags = 0;
  if(flip_vertical && flip_vertical->as<yaml::Scalar>()->b()) flags |= Image::FlipVertical;

  auto view = manager().mapLocation(location);
  if(!view) throw IOError(location->repr());

  auto img = Image::from_file(view.get(), view.size(), num_channels, flags, id, name->str(), path->str());

  if(img->as<Image>()->dimensions() != ivec2{ width, height }) {
    throw InvalidResourceError(id);
  }

  return img;
}

Resource::Ptr SimpleFsLoader::loadMesh(Resource::Id id, const yaml::Document& meta)
{
  const yaml::Scalar *name, *path, *location;
  std::tie(name, path, location) = name_path_location(meta);

  auto view = manager().mapLocation(location);
  if(!view) throw IOError(location->repr());

  return Mesh::from_yaml(view, meta, id, name->str(), path->str());
}

}
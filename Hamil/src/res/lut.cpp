#include <res/lut.h>

#include <yaml/document.h>
#include <yaml/node.h>

#include <unordered_map>

namespace res {

static const std::unordered_map<std::string, LookupTable::DataType> p_datatypes = {
  { "r16f", LookupTable::r16f }, { "r32f", LookupTable::r32f },
  { "rgba16f", LookupTable::rgba16f }, { "rgba32f", LookupTable::rgba32f },
};

Resource::Ptr LookupTable::from_yaml(IOBuffer lut_data,
  const yaml::Document& doc, Id id,
  const std::string& name, const std::string& path)
{
  auto lut = new LookupTable(id, LookupTable::tag(), name, File, path);
  
  auto it = p_datatypes.find(doc("type")->as<yaml::Scalar>()->str());
  if(it == p_datatypes.end()) throw Error();  // invalid DataType

  lut->m_type = it->second;
  lut->m_lut = lut_data;
  lut->m_loaded = true;

  return Resource::Ptr(lut);
}

LookupTable::DataType LookupTable::type() const
{
  return m_type;
}

void *LookupTable::data()
{
  return m_lut.get<void>();
}

size_t LookupTable::size() const
{
  return m_lut.size();
}

}
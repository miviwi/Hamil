#include <yaml/node.h>
#include <util/hash.h>

#include <yaml.h>

#include <cstdlib>
#include <cstring>
#include <unordered_map>
#include <regex>

namespace yaml {

Node::Node(Type type, Tag tag) :
  m_type(type), m_tag(tag)
{
  if(!m_tag) return;
}

Node::~Node()
{
}

Node::Type Node::type() const
{
  return m_type;
}

const Node::Tag& Node::tag() const
{
  return m_tag;
}

std::string Node::tagString() const
{
  if(!m_tag) return "";

  auto value = m_tag.value();
  return value.front() == '!' ? value: "!!"+value;
}

bool Node::Compare::operator()(const Node::Ptr& a, const Node::Ptr& b) const
{
  return a->compare(b);
}

Scalar::Scalar(byte *data, size_t sz, Tag tag) :
  Node(NodeType, tag), m_data(sz, '\0')
{
  memcpy(m_data.data(), data, sz);
}

Scalar::Scalar(const std::string& str, Tag tag) :
  Node(NodeType, tag), m_data(str.length(), '\0')
{
  memcpy(m_data.data(), str.c_str(), str.length());
}

Node::Ptr Scalar::from_str(const std::string& str)
{
  return std::make_shared<Scalar>(str);
}

Node::Ptr Scalar::from_i(long long i)
{
  return std::make_shared<Scalar>(std::to_string(i));
}

Node::Ptr Scalar::from_ui(unsigned long long ui)
{
  return std::make_shared<Scalar>(std::to_string(ui));
}

Node::Ptr Scalar::from_f(double f)
{
  return std::make_shared<Scalar>(std::to_string(f));
}

Node::Ptr Scalar::from_b(bool b)
{
  return std::make_shared<Scalar>(b ? "yes" : "no");
}

Node::Ptr Scalar::from_null()
{
  return std::make_shared<Scalar>("null");
}

std::string Scalar::repr() const
{
  auto val = size() ? str() : "null";

  return tag() ? tagString()+" "+val : val;
}

// Must be kept in sync with Scalar::DataType!
// 
// The regex patterns have been taken from:
//   - yaml.org/type/null.html
//   - yaml.org/type/int.html
//   - yaml.org/type/float.html
//   - yaml.org/type/bool.html
static const std::regex p_datatype_regexes[] = {
  // Scalar::Null
  std::regex("~|null",                            std::regex::icase|std::regex::optimize), 

  // Scalar::Int
  std::regex("[-+]?0b[0-1_]+|[-+]?0[0-7_]+"
             "|[-+]?(0|[1-9][0-9_]*)"
             "|[-+]?0x[0-9a-fA-F_]+"
             "|[-+]?[1-9][0-9_]*(:[0-5]?[0-9])+", std::regex::optimize),

  // Scalar::Float
  std::regex("[-+]?([0-9][0-9_]*)?\\.[0-9.]*([eE][-+][0-9]+)?"
             "|[-+]?[0-9][0-9_]*(:[0-5]?[0-9])+\\.[0-9_]*"
             "|[-+]?\\.(inf|Inf|INF)"
             "|\\.(nan|NaN|NAN)",                 std::regex::optimize),

  // Scalar::Boolean
  std::regex("(yes|true|on|y)"
             "|(no|false|off|n)",                 std::regex::icase|std::regex::optimize),

  // Scalar::String
  std::regex(".*",                                std::regex::optimize),
};

#define YAML_CORE_URI "tag:yaml.org,2002:"

static const std::unordered_map<std::string, Scalar::DataType> p_core_types = {
  { YAML_CORE_URI "null",  Scalar::Null    },
  { YAML_CORE_URI "int",   Scalar::Int     },
  { YAML_CORE_URI "float", Scalar::Float   },
  { YAML_CORE_URI "bool",  Scalar::Boolean },
  { YAML_CORE_URI "str",   Scalar::String  },
};

Scalar::DataType Scalar::dataType() const
{
  if(tag()) {
    auto val = tag().value();
    if(val.front() == '!') return Scalar::Tagged;

    auto it = p_core_types.find(val);
    if(it == p_core_types.end()) return Scalar::Invalid;

    return it->second;
  }

  std::string s = str();
  for(int i = 0; i < NumDataTypes; i++) {
    if(!std::regex_match(s, p_datatype_regexes[i])) continue;

    return (DataType)i;
  }

  return Scalar::Invalid; // should never be reached
}

const char *Scalar::str() const
{
  return (const char *)m_data.data();
}

long long Scalar::i() const
{
  if(auto val = probeCache<long long>()) return *val;
  return fillCache(strtoll(str(), nullptr, 0));
}

unsigned long long Scalar::ui() const
{
  if(auto val = probeCache<unsigned long long>()) return *val;
  return fillCache(strtoull(str(), nullptr, 0));
}

double Scalar::f() const
{
  if(auto val = probeCache<double>()) return *val;
  return fillCache(strtod(str(), nullptr));
}

bool Scalar::b() const
{
  if(auto val = probeCache<bool>()) return *val;

  std::match_results<const char *> matches;
  if(!std::regex_match(str(), matches, p_datatype_regexes[Scalar::Boolean])) return false;

  return fillCache(matches[1].matched); // matches[0] is the whole pattern
                                        // matches[1..n] are the capture groups
}

bool Scalar::null() const
{
  return m_data.size() == 4 && memcmp(m_data.data(), "null", 4);
}

size_t Scalar::size() const
{
  return m_data.size();
}

size_t Scalar::hash() const
{
  return util::hash(m_data.data(), m_data.size());
}

bool Scalar::compare(const Node::Ptr& other) const
{
  if(other->type() != NodeType) return false;

  auto n = (const Scalar *)other.get();
  return n->size() == size() && memcmp(m_data.data(), n->m_data.data(), size()) == 0;
}

Sequence::Sequence(Tag tag) :
  Node(NodeType, tag)
{
}

std::string Sequence::repr() const
{
  std::string str = tag() ? tagString()+" [" : "[";
  for(auto& item : m) {
    str += item->repr() + ", ";
  }
  if(!m.empty()) str.resize(str.size()-2); // Remove the trailing ', '

  return str += "]";
}

void Sequence::append(Node::Ptr node)
{
  m.push_back(node);
}

Node::Ptr Sequence::get(size_t idx) const
{
  return m[idx];
}

size_t Sequence::size() const
{
  return m.size();
}

size_t Sequence::hash() const
{
  size_t seed = 0;
  for(auto& item : m) util::hash_combine<Node::Hash>(seed, item);

  return seed;
}

bool Sequence::compare(const Node::Ptr& other) const
{
  if(other->type() != NodeType) return false;

  auto n = (const Sequence *)other.get();
  if(n->size() != size()) return false;

  for(size_t i = 0; i < size(); i++) {
    const auto& a = m[i], b = n->m[i];

    if(!a->compare(b)) return false;
  }

  return true;
}

Mapping::Mapping(Tag tag) :
  Node(NodeType, tag)
{
}

std::string Mapping::repr() const
{
  std::string str = tag() ? tagString()+" {" : "{";
  for(auto& item : m) {
    str += item.first->repr() + ": " + item.second->repr() + ", ";
  }
  if(!m.empty()) str.resize(str.size()-2); // Remove the trailing ', '

  return str += "}";
}

void Mapping::append(Node::Ptr key, Node::Ptr value)
{
  m.insert({ key, value });
}

Node::Ptr Mapping::get(const std::string& key) const
{
  return get(Scalar::from_str(key));
}

Node::Ptr Mapping::get(Node::Ptr key) const
{
  return m.find(key)->second;
}

size_t Mapping::size() const
{
  return m.size();
}

size_t Mapping::hash() const
{
  size_t seed = 0;
  for(auto& item : m) {
    util::hash_combine<Node::Hash>(seed, item.first);
    util::hash_combine<Node::Hash>(seed, item.second);
  }

  return seed;
}

bool Mapping::compare(const Node::Ptr& other) const
{
  if(other->type() != NodeType) return false;

  auto n = (const Mapping *)other.get();
  if(n->size() != size()) return false;

  for(auto& item : m) {
    const auto& key = item.first,
      value = item.second;

    auto it = n->m.find(key);
    if(it == n->m.end() || !value->compare(it->second)) return false;
  }

  return true;
}

}
#include <yaml/node.h>
#include <util/hash.h>

#include <yaml.h>

#include <cstdlib>
#include <cstring>
#include <unordered_map>
#include <regex>

namespace yaml {

Node::Node(Type type, Tag tag, Style style) :
  m_type(type), m_tag(tag), m_style(style)
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

void Node::tag(const Tag& t)
{
  m_tag = t;
}

std::string Node::tagString() const
{
  if(!m_tag) return "";

  auto value = m_tag.value();
  return value.front() == '!' ? value: "!!"+value;
}

Node::Style Node::style() const
{
  return m_style;
}

bool Node::Compare::operator()(const Node::Ptr& a, const Node::Ptr& b) const
{
  return a->compare(b);
}

Scalar::Scalar(byte *data, size_t sz, Tag tag, Style style) :
  Node(NodeType, tag, style), m_data(sz, '\0'), m_cache(std::monostate())
{
  memcpy(m_data.data(), data, sz);
}

Scalar::Scalar(const std::string& str, Tag tag, Style style) :
  Node(NodeType, tag, style), m_data(str.length(), '\0'), m_cache(std::monostate())
{
  memcpy(m_data.data(), str.data(), str.length());
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
// The Scalar::Int,Scalar::Float regex patterns come from:
//   - yaml.org/type/int.html
//   - yaml.org/type/float.html
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

static const std::unordered_map<std::string, Scalar::DataType> p_core_types = {
  { YAML_NULL_TAG,  Scalar::Null    },
  { YAML_INT_TAG,   Scalar::Int     },
  { YAML_FLOAT_TAG, Scalar::Float   },
  { YAML_BOOL_TAG,  Scalar::Boolean },
  { YAML_STR_TAG,   Scalar::String  },
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

Sequence::Sequence(Tag tag, Style style) :
  Node(NodeType, tag, style)
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

Sequence *Sequence::append(const Node::Ptr &node)
{
  m.push_back(node);

  return this;
}

Node::Ptr Sequence::get(size_t idx) const
{
  return idx < size() ? m[idx] : Node::Ptr();
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

void Sequence::foreach(IterFn fn) const
{
  for(const auto& value : m) fn(value);
}

void Sequence::foreach(KVIterFn fn) const
{
  for(size_t i = 0; i < m.size(); i++) fn(Scalar::from_ui(i), m[i]);
}

Mapping::Mapping(Tag tag, Style style) :
  Node(NodeType, tag, style)
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

Mapping *Mapping::append(Node::Ptr key, Node::Ptr value)
{
  m.emplace(key, value);

  if(m_ordered) m_ordered->emplace_back(key, value);

  return this;
}

void Mapping::retainOrder(bool enable)
{
  if(enable) {
    if(m_ordered) return;

    m_ordered.reset(new NodeVector(m.begin(), m.end()));
  } else {
    m_ordered.reset();
  }
}

bool Mapping::ordered() const
{
  return m_ordered.get();
}

Node::Ptr Mapping::get(const std::string& key) const
{
  return get(Scalar::from_str(key));
}

Node::Ptr Mapping::get(Node::Ptr key) const
{
  auto it = m.find(key);
  return it != m.end() ? it->second : Node::Ptr();
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

void Mapping::foreach(IterFn fn) const
{
  if(m_ordered) {
    for(const auto& item : *m_ordered) fn(item.first);
  } else {
    for(const auto& item : m) fn(item.first);
  }
}

void Mapping::foreach(KVIterFn fn) const
{
  if(m_ordered) {
    for(const auto& item : *m_ordered) fn(item.first, item.second);
  } else {
    for(const auto& item : m) fn(item.first, item.second);
  }
}

}
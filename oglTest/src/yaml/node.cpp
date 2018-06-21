#include <yaml/node.h>
#include <util/hash.h>

#include <yaml.h>

#include <regex>
#include <cstdlib>
#include <cstring>

namespace yaml {

Node::Node(Type type) :
  m_type(type)
{
}

Node::~Node()
{
}

Node::Type Node::type() const
{
  return m_type;
}

bool Node::Compare::operator()(const Node::Ptr& a, const Node::Ptr& b) const
{
  return a->compare(b);
}

Scalar::Scalar(byte *data, size_t sz) :
  Node(NodeType), m_data(sz+1)
{
  memcpy(m_data.data(), data, sz);
}

Node::Ptr Scalar::from_str(const std::string& str)
{
  return std::make_shared<Scalar>(str.c_str(), str.length());
}

Node::Ptr Scalar::from_i(long long i)
{
  auto str = std::to_string(i);
  return std::make_shared<Scalar>(str.c_str(), str.length());
}

Node::Ptr Scalar::from_ui(unsigned long long ui)
{
  auto str = std::to_string(ui);
  return std::make_shared<Scalar>(str.c_str(), str.length());
}

Node::Ptr Scalar::from_f(double f)
{
  auto str = std::to_string(f);
  return std::make_shared<Scalar>(str.c_str(), str.length());
}

Node::Ptr Scalar::from_b(bool b)
{
  std::string str = b ? "yes" : "no";
  return std::make_shared<Scalar>(str.c_str(), str.length());
}

std::string Scalar::repr() const
{
  return str();
}

const char *Scalar::str() const
{
  return (const char *)m_data.data();
}

long long Scalar::i() const
{
  if(auto val = probeCache<long long>()) return *val;
  
  char *end = nullptr;
  return fillCache(strtoll(str(), &end, 0));
}

unsigned long long Scalar::ui() const
{
  if(auto val = probeCache<unsigned long long>()) return *val;

  char *end = nullptr;
  return fillCache(strtoull(str(), &end, 0));
}

double Scalar::f() const
{
  if(auto val = probeCache<double>()) return *val;

  char *end = nullptr;
  return fillCache(strtod(str(), &end));
}

static const std::regex p_bool_regex("(yes|true|on|y)|(no|false|off|n)", std::regex::icase);
bool Scalar::b() const
{
  if(auto val = probeCache<bool>()) return *val;

  std::match_results<const char *> matches;
  if(!std::regex_match(str(), matches, p_bool_regex)) return false;

  return fillCache(matches[1].matched); // matches[0] is the whole pattern
                                        // matches[1] is the truthy group
                                        // matches[2] is the falsy group
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

Sequence::Sequence() :
  Node(NodeType)
{
}

std::string Sequence::repr() const
{
  std::string str = "[";
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

Mapping::Mapping() :
  Node(NodeType)
{
}

std::string Mapping::repr() const
{
  std::string str = "{";
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
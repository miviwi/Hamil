#pragma once

#include <common.h>

#include <string>
#include <vector>
#include <unordered_map>
#include <utility>
#include <functional>
#include <variant>
#include <optional>
#include <memory>
#include <type_traits>
#include <initializer_list>

namespace yaml {

using char_t = byte;
using string = std::basic_string<char_t>;

class Node {
public:
  using Ptr = std::shared_ptr<Node>;
  using Tag = std::optional<std::string>;
  using IterFn   = std::function<void(Node::Ptr /* Sequence value/Mapping key */)>;
  using KVIterFn = std::function<void(Node::Ptr key, Node::Ptr value)>;

  enum Type {
    Invalid, Scalar, Sequence, Mapping
  };

  enum Style {
    Any,

    // Mapping/Sequence styles
    Flow, Block,

    // Scalar styles
    Plain,
    Literal, Folded,
    SingleQuoted, DoubleQuoted,
  };

  Node(Type type, Tag tag, Style style = Any);
  virtual ~Node();

  Type type() const;

  const Tag& tag() const;
  void tag(const Tag& t);
  std::string tagString() const;

  Style style() const;

  template <typename T>
  const T *as() const
  {
    return type() == T::NodeType ? (const T *)this : nullptr;
  }

  virtual std::string repr() const = 0;

  virtual Node::Ptr get(const std::string& key) const = 0;
  virtual Node::Ptr get(size_t idx) const = 0;

  template <typename T, typename U>
  const T *get(const U& key) const
  {
    return get(key)->as<T>();
  }

  virtual size_t hash() const = 0;
  virtual bool compare(const Node::Ptr& other) const = 0;

  struct Hash { size_t operator()(const Node::Ptr& n) const { return n->hash(); } };
  struct Compare { bool operator()(const Node::Ptr& a, const Node::Ptr& b) const; };

  virtual void foreach(IterFn fn) const   { }
  virtual void foreach(KVIterFn fn) const { }

private:
  Type m_type;
  Tag m_tag;
  Style m_style;
};

class Scalar : public Node {
public:
  static constexpr Type NodeType = Node::Scalar;

  enum DataType {
    Null, Int, Float, Boolean, String,

    NumDataTypes,
    Tagged, Invalid = ~0,
  };
  
  Scalar(byte *data, size_t sz, Tag tag = {}, Style style = Any);
  Scalar(const std::string& str, Tag tag = {}, Style style = Any);

  static Node::Ptr from_str(const std::string& str);
  static Node::Ptr from_i(long long i);
  static Node::Ptr from_ui(unsigned long long ui);
  static Node::Ptr from_f(double f);
  static Node::Ptr from_b(bool b);
  static Node::Ptr from_null();

  virtual std::string repr() const;

  virtual Node::Ptr get(const std::string& key) const { return Node::Ptr(); }
  virtual Node::Ptr get(size_t idx) const { return Node::Ptr(); }

  DataType dataType() const;

  const char *str() const;
  long long i() const;
  unsigned long long ui() const;
  double f() const;
  bool b() const;
  bool null() const;

  size_t size() const; // returns the size of the string/data

  virtual size_t hash() const;
  virtual bool compare(const Node::Ptr& other) const;

private:
  template <typename T> T *probeCache() const { return std::get_if<T>(&m_cache); }
  template <typename T> T fillCache(T val) const { m_cache = val; return val; }

  string m_data;

  using CacheStorage = std::variant<
    std::monostate, long long, unsigned long long, double, bool
  >;
  mutable CacheStorage m_cache;
 };

class Sequence : public Node {
public:
  static constexpr Type NodeType = Node::Sequence;

  using Seq = std::vector<Node::Ptr>;
  using Iterator = Seq::const_iterator;
  Sequence(Tag tag = {}, Style = Any);

  virtual std::string repr() const;

  // Returns 'this'
  Sequence *append(const Node::Ptr &node);

  // Returns 'this
  Sequence *concat(const Sequence *seq);
  Sequence *concat(const Node::Ptr& node);

  virtual Node::Ptr get(const std::string& key) const { return Node::Ptr(); }
  virtual Node::Ptr get(size_t idx) const;
  Node::Ptr operator[](size_t idx) const { return get(idx); }

  template <typename T>
  const T *get(size_t key) const
  {
    return get(key)->as<T>();
  }

  size_t size() const;

  virtual size_t hash() const;
  virtual bool compare(const Node::Ptr& other) const;

  Iterator begin() const { return m.cbegin(); }
  Iterator end()   const { return m.cend(); }

  virtual void foreach(IterFn fn) const;
  virtual void foreach(KVIterFn fn) const; // SLOW!

private:
  Seq m;
};

class Mapping : public Node {
public:
  static constexpr Type NodeType = Node::Mapping;

  using Map = std::unordered_map<Node::Ptr, Node::Ptr, Node::Hash, Node::Compare>;
  Mapping(Tag tag = {}, Style style = Any);

  virtual std::string repr() const;

  // Returns 'this'
  Mapping *append(Node::Ptr key, Node::Ptr value);

  // Returns 'this'
  Mapping *concat(const Mapping *map);
  Mapping *concat(const Node::Ptr& node);

  // Makes the mapping retain the insertion order
  //   of the keys when iterating
  void retainOrder(bool enable = true);
  bool ordered() const;

  virtual Node::Ptr get(const std::string& key) const;
  virtual Node::Ptr get(size_t idx) const { return Node::Ptr(); }
  virtual Node::Ptr get(Node::Ptr key) const;
  Node::Ptr operator[](Node::Ptr key) const { return get(key); }

  template <typename T, typename U>
  const T *get(const U& key) const
  {
    return get(key)->as<T>();
  }

  size_t size() const;

  virtual size_t hash() const;
  virtual bool compare(const Node::Ptr& other) const;

  virtual void foreach(IterFn fn) const;
  virtual void foreach(KVIterFn fn) const;

private:
  Map m;

  using NodeVector = std::vector<std::pair<Node::Ptr, Node::Ptr>>;
  std::unique_ptr<NodeVector> m_ordered;
};

static Mapping *string_mapping(std::initializer_list<std::pair<std::string, std::string>> items)
{
  auto map = new Mapping;
  for(const auto& item : items) {
    map->append(
      Scalar::from_str(item.first),
      Scalar::from_str(item.second)
    );
  }

  return map;
}

static Mapping *ordered_string_mapping(std::initializer_list<std::pair<std::string, std::string>> items)
{
  auto map = new Mapping;
  map->retainOrder();

  for(const auto& item : items) {
    map->append(
      Scalar::from_str(item.first),
      Scalar::from_str(item.second)
    );
  }

  return map;
}

static Sequence *isequence(std::initializer_list<long long> items)
{
  auto seq = new Sequence;

  for(const auto& item : items) {
    seq->append(Scalar::from_i(item));
  }

  return seq;
}

}
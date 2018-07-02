#pragma once

#include <common.h>

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <variant>
#include <optional>
#include <memory>
#include <type_traits>

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

  Node(Type type, Tag tag);
  virtual ~Node();

  Type type() const;

  const Tag& tag() const;
  std::string tagString() const;

  template <typename T>
  const T *as() const
  {
    return type() == T::NodeType ? (const T *)this : nullptr;
  }

  virtual std::string repr() const = 0;

  virtual Node::Ptr get(const std::string& key) const = 0;
  virtual Node::Ptr get(size_t idx) const = 0;

  virtual size_t hash() const = 0;
  virtual bool compare(const Node::Ptr& other) const = 0;

  struct Hash { size_t operator()(const Node::Ptr& n) const { return n->hash(); } };
  struct Compare { bool operator()(const Node::Ptr& a, const Node::Ptr& b) const; };

  virtual void foreach(IterFn fn) { }
  virtual void foreach(KVIterFn fn) { }

private:
  Type m_type;
  Tag m_tag;
};

class Scalar : public Node {
public:
  static constexpr Type NodeType = Node::Scalar;

  enum DataType {
    Null, Int, Float, Boolean, String,

    NumDataTypes,
    Tagged, Invalid = ~0,
  };
  
  Scalar(byte *data, size_t sz, Tag tag = {});
  Scalar(const std::string& str, Tag tag = {});

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
    long long, unsigned long long, double, bool
  >;
  mutable CacheStorage m_cache;
 };

class Sequence : public Node {
public:
  static constexpr Type NodeType = Node::Sequence;

  using Seq = std::vector<Node::Ptr>;
  using Iterator = Seq::iterator;
  Sequence(Tag tag = {});

  virtual std::string repr() const;

  void append(const Node::Ptr &node);

  virtual Node::Ptr get(const std::string& key) const { return Node::Ptr(); }
  virtual Node::Ptr get(size_t idx) const;
  Node::Ptr operator[](size_t idx) const { return get(idx); }

  size_t size() const;

  virtual size_t hash() const;
  virtual bool compare(const Node::Ptr& other) const;

  Iterator begin() { return m.begin(); }
  Iterator end()   { return m.end(); }

  virtual void foreach(IterFn fn);
  virtual void foreach(KVIterFn fn); // SLOW!

private:
  Seq m;
};

class Mapping : public Node {
public:
  static constexpr Type NodeType = Node::Mapping;

  using Map = std::unordered_map<Node::Ptr, Node::Ptr, Node::Hash, Node::Compare>;
  using Iterator = Map::iterator;
  Mapping(Tag tag = {});

  virtual std::string repr() const;

  void append(Node::Ptr key, Node::Ptr value);

  virtual Node::Ptr get(const std::string& key) const;
  virtual Node::Ptr get(size_t idx) const { return Node::Ptr(); }
  virtual Node::Ptr get(Node::Ptr key) const;
  Node::Ptr operator[](Node::Ptr key) const { return get(key); }

  size_t size() const;

  virtual size_t hash() const;
  virtual bool compare(const Node::Ptr& other) const;

  virtual void foreach(IterFn fn);
  virtual void foreach(KVIterFn fn);

  Iterator begin() { return m.begin(); }
  Iterator end()   { return m.end(); }

private:
  Map m;
};

}
#pragma once

#include <common.h>

#include <string>
#include <vector>
#include <unordered_map>
#include <variant>
#include <memory>
#include <type_traits>

namespace yaml {

class Node {
public:
  using Ptr = std::shared_ptr<Node>;

  enum Type {
    Invalid, Scalar, Sequence, Mapping
  };

  Node(Type type);
  virtual ~Node();

  Type type() const;

  template <typename T>
  const T *as() const
  {
    return type() == T::NodeType ? (const T *)this : nullptr;
  }

  virtual std::string repr() const = 0;

  virtual size_t hash() const = 0;
  virtual bool compare(const Node::Ptr& other) const = 0;

  struct Hash { size_t operator()(const Node::Ptr& n) const { return n->hash(); } };
  struct Compare { bool operator()(const Node::Ptr& a, const Node::Ptr& b) const; };

private:
  Type m_type;
};

class Scalar : public Node {
public:
  static constexpr Type NodeType = Node::Scalar;
  
  Scalar(byte *data, size_t sz);

  static Node::Ptr from_str(const std::string& str);
  static Node::Ptr from_i(long long i);
  static Node::Ptr from_ui(unsigned long long ui);
  static Node::Ptr from_f(double f);
  static Node::Ptr from_b(bool b);

  virtual std::string repr() const;

  const char *str() const;

  long long i() const;
  unsigned long long ui() const;
  double f() const;
  bool b() const;

  size_t size() const; // returns the size of the string/data

  virtual size_t hash() const;
  virtual bool compare(const Node::Ptr& other) const;

private:
  template <typename T> T *probeCache() const { return std::get_if<T>(&m_cache); }
  template <typename T> T fillCache(T val) const { m_cache = val; return val; }

  std::vector<byte> m_data;

  using CacheStorage = std::variant<
    long long, unsigned long long, double, bool
  >;
  mutable CacheStorage m_cache;
};

class Sequence : public Node {
public:
  static constexpr Type NodeType = Node::Sequence;
  
  Sequence();

  virtual std::string repr() const;

  void append(Node::Ptr node);

  Node::Ptr get(size_t idx) const;
  Node::Ptr operator[](size_t idx) const { return get(idx); }

  size_t size() const;

  virtual size_t hash() const;
  virtual bool compare(const Node::Ptr& other) const;

private:
  std::vector<Node::Ptr> m;
};

class Mapping : public Node {
public:
  static constexpr Type NodeType = Node::Mapping;
  
  Mapping();

  virtual std::string repr() const;

  void append(Node::Ptr key, Node::Ptr value);

  Node::Ptr get(Node::Ptr key) const;
  Node::Ptr operator[](Node::Ptr key) const { return get(key); }

  size_t size() const;

  virtual size_t hash() const;
  virtual bool compare(const Node::Ptr& other) const;

private:
  std::unordered_map<Node::Ptr, Node::Ptr, Node::Hash, Node::Compare> m;
};

}
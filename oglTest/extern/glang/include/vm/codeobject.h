#pragma once

#include <cstdint>

namespace glang {

namespace assembler {

struct CodeObject {
public:
  enum { InitialAlloc = 4096 };

  CodeObject();
  CodeObject(const CodeObject& other);

  CodeObject& operator=(const CodeObject& other) = delete;

  ~CodeObject();

  void ref();

  void push(const void *data, size_t sz);
  void *data() const { return m_code; }

  size_t size() const { return m_sz; }

  size_t hash() const;
  bool operator==(const CodeObject& other) const;

private:
  unsigned char *ptr() const { return ((unsigned char *)m_code)+m_sz; }

  long *m_ref;

  size_t m_sz, m_alloc;
  void *m_code;
};

}

}

namespace std {

template <typename T> struct hash;

template <>
struct hash<glang::assembler::CodeObject> {
  typedef glang::assembler::CodeObject argument_type;
  typedef size_t result_type;

  result_type operator()(argument_type const& s) const
  {
    return s.hash();
  }
};

}
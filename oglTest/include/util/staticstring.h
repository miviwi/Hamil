#pragma once

#include <common.h>

namespace util {

class StaticString {
public:
  template <size_t N>
  constexpr StaticString(const char (&str)[N]) :
    m_str(str), m_sz(N)
  { }
  constexpr StaticString() :
    StaticString("")
  { }

  constexpr const char *get() const { return m_str; }
  constexpr size_t size() const { return m_sz; }

  size_t hash() const;

  struct Hash { size_t operator()(const StaticString& str) const { return str.hash(); } };

private:
  const char *m_str;
  size_t m_sz;
};

inline bool operator==(const StaticString& a, const StaticString& b)
{
  return a.get() == b.get();
}

}

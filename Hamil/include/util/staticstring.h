#pragma once

#include <common.h>

#include <string>
#include <string_view>
#include <type_traits>

namespace util {

class StaticString {
public:
  template <size_t N>
  constexpr StaticString(const char (&str)[N]) :
    m_str(str, N)
  { }
  constexpr StaticString() :
    StaticString("")
  { }

  constexpr const char *get() const { return m_str.data(); }
  constexpr size_t size() const { return m_str.size(); }

  constexpr bool eq(const StaticString& other) const
  {
    return m_str.compare(other.m_str);
  }

  size_t hash() const;

  struct Hash { size_t operator()(const StaticString& str) const { return str.hash(); } };

private:
  const std::string_view m_str;
};

inline bool operator==(const StaticString& a, const StaticString& b)
{
  return a.get() == b.get();
}

inline bool operator==(const StaticString& a, const std::string& b)
{
  return a.get() == b;
}

inline bool operator==(const std::string& a, const StaticString& b)
{
  return a == b.get();
}

}

namespace std {

template <>
struct hash<util::StaticString> {
public:
  size_t operator()(const util::StaticString& s) const { return s.hash(); }
};

}

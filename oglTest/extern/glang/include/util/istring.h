#pragma once

#include <cstdint>
#include <utility>
#include <string>
#include <functional>
#include <unordered_set>

namespace glang {

class __declspec(dllexport) InternedString {
public:
  using Str = std::pair<std::string, size_t>;

  InternedString() : InternedString("") { }
  InternedString(const char *str) : InternedString(std::string(str)) { }
  InternedString(const std::string& str);

  const char *ptr() const { return str().c_str(); }

  size_t size() const { return str().size(); }

  bool operator==(const InternedString& other) const { return m_str == other.m_str; }
  bool operator!=(const InternedString& other) const { return m_str != other.m_str; }

  size_t hash() const { return m_str->second; }

  operator const std::string&() { return str(); }

  struct __declspec(dllexport) HashStr {
    size_t operator()(Str const& s) const { return s.second; }
  };
  using Store = std::unordered_set<Str, HashStr>;

private:
  const Str *m_str;

  const std::string& str() const { return m_str->first; }
};

}

namespace std {

template <>
struct __declspec(dllexport) hash<glang::InternedString> {
  typedef glang::InternedString argument_type;
  typedef size_t result_type;

  result_type operator()(argument_type const& s) const
  {
    return s.hash();
  }
};

}

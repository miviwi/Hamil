#pragma once

#include <common.h>

#include <string>
#include <unordered_map>
#include <set>
#include <optional>
#include <variant>
#include <type_traits>

namespace util {

class Option {
public:
  enum Type : size_t {
    // These values correspond directly to m_val::index()
    Bool, Int, String,

    Invalid = std::variant_npos,
  };

  enum Flags {
    Default   = 0,
    Required  = 1<<0, // Cannot be set if the type == Bool
    ShortName = 1<<1, // Option has short form (first character of name)
  };

  struct Error { };

  struct NameError : public Error {
    const std::string name;
    NameError(const std::string& name_) :
      name(name_)
    { }
  };
  struct ShortNameAmbiguousError : public NameError {
    using NameError::NameError;
  };

  struct AssignError : public Error {
    const Type expected, gotten;
    AssignError(Type expected_, Type gotten_) :
      expected(expected_), gotten(gotten_)
    { }
  };

  struct TypeError : public Error { };

  struct FlagError : public Error {
    const std::string what;
    FlagError(const std::string& what_) :
      what(what_)
    { }
  };

  Option(Type type, const std::string& doc = "", Flags flags = Default);

  bool operator==(const Option& other) const;

  Type type() const;
  Flags flags() const;
  const std::string& doc() const;

  operator bool() const;

  bool b() const;
  long i() const;
  const std::string& str() const;

private:
  friend class ConsoleOpts;

  Option(bool b);
  Option(long i);
  Option(const std::string& str);

  template <typename T>
  Option& operator=(const T& val)
  {
    static_assert(std::is_same_v<T, bool> || std::is_same_v<T, long> || std::is_same_v<T, std::string>,
      "Invalid type given to Option::operator=()!");

    m_val = val;
    if(m_val.index() != m_type) throw AssignError(m_type, m_val.index());

    return *this;
  }

  Flags m_flags;
  Type m_type;
  std::string m_doc;
  std::variant<bool, long, std::string> m_val;
};

// Commandline option parser, usage:
//     auto opts =
//       ConsoleOpts()
//         .boolean("b", "a boolean option")
//         .string("str", "a string option", Option::Optional);
//
//     if(opts.parse(argc, argv)) panic(...);
//
// A valid option name starts with a lowercase letter followed by
//   lower-case alphanumerics separated by hyphens
// It is specified with a double-hyphen or with a single one and
//   the first character if the Option::ShortName flags was set
class ConsoleOpts {
public:
  enum {
    DocNameMargin = 25,
  };

  struct Error {
    const std::string what;
    Error(const std::string& what_) :
      what(what_)
    { }
  };

  struct ParseError : public Error {
    using Error::Error;
  };

  ConsoleOpts& boolean(const std::string& name, const std::string& doc = "", Option::Flags flags = Option::Default);
  ConsoleOpts& integer(const std::string& name, const std::string& doc = "", Option::Flags flags = Option::Default);
  ConsoleOpts& string(const std::string& name, const std::string& doc = "", Option::Flags flags = Option::Default);

  // returns the name of the first option which failed validation
  //   or std::nullopt if validation succeded
  // i.e. the return value is TRUE when validation fails
  std::optional<std::string> parse(int argc, char *argv[]);

  // returns a pointer if option 'name' was supplied,
  //   otherwise returns nullptr
  const Option *get(const std::string& name);

  // returns a description of all the options
  std::string doc() const;

private:
  ConsoleOpts& option(const std::string& name, Option&& opt);

  std::unordered_map<std::string, Option> m_opts;
  std::unordered_map<char, std::string> m_short_names;
};

}
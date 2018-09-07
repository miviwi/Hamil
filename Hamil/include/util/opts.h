#pragma once

#include <common.h>

#include <string>
#include <unordered_map>
#include <set>
#include <functional>
#include <optional>
#include <variant>
#include <type_traits>

namespace util {

class Option {
public:
  // The values correspond directly to m_val::index()
  enum Type : size_t {
    Empty,

    Bool, Int, String, List,
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

  using StringList = std::vector<std::string>;

  Option(Type type, const std::string& doc = "", Flags flags = Default);

  bool operator==(const Option& other) const;

  Type type() const;
  Flags flags() const;
  const std::string& doc() const;

  // returns 'true' when the Option has a value
  operator bool() const;

  // returns the Option's boolean value (throws an exception when type() != Option::Bool)
  bool b() const;
  // returns the Option's integer value (throws an exception when type() != Option::Int)
  long i() const;
  // returns the Option's string value (throws an exception when type() != Option::String)
  const std::string& str() const;
  // returns the Option's StringList value (throws an exception when type() != Option::List)
  const StringList& list() const;


private:
  friend class ConsoleOpts;

  template <typename T>
  Option& operator=(const T& val)
  {
    static_assert(
         std::is_same_v<T, bool>
      || std::is_same_v<T, long>
      || std::is_same_v<T, std::string>,
      "Invalid type given to Option::operator=()!"
    );

    m_val = val;
    if(m_val.index() != m_type) throw AssignError(m_type, (Type)m_val.index());

    return *this;
  }

  // set Option::List value
  void list(const std::string& str);

  Flags m_flags;
  Type m_type;
  std::string m_doc;
  std::variant<std::monostate, bool, long, std::string, StringList> m_val;
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
    DocNameMargin = 35,
  };

  using IterFn = std::function<void(const std::string& name, const Option& opt)>;

  struct Error {
    const std::string what;
    Error(const std::string& what_) :
      what(what_)
    { }
  };

  struct ParseError : public Error {
    using Error::Error;
  };
  struct InvalidOptionError : public ParseError {
    InvalidOptionError(const std::string& name) :
      ParseError("no such option '" + name + "'")
    { }
  };
  struct MissingValueError : public ParseError {
    MissingValueError(const std::string& name) :
      ParseError("'" + name + "' requires a value")
    { }
  };

  ConsoleOpts& boolean(const std::string& name,
    const std::string& doc = "", Option::Flags flags = Option::Default);
  ConsoleOpts& integer(const std::string& name,
    const std::string& doc = "", Option::Flags flags = Option::Default);
  ConsoleOpts& string(const std::string& name,
    const std::string& doc = "", Option::Flags flags = Option::Default);
  ConsoleOpts& list(const std::string& name,
    const std::string& doc = "", Option::Flags flags = Option::Default);

  // returns the name of the first required option not present
  //   or std::nullopt if parsing succeded
  // i.e. the return value is TRUE when parsing fails
  std::optional<std::string> parse(int argc, char *argv[]);

  // returns a pointer if option 'name' was supplied,
  //   otherwise returns nullptr
  const Option *get(const std::string& name) const;
  const Option *operator()(const std::string& name) const { return get(name); }

  // iterates over all the supplied options
  //   - booleans always count as "supplied"
  void foreach(IterFn fn) const;

  // returns a description of all the options
  std::string doc() const;

  void dbg_PrintOpts() const;

private:
  ConsoleOpts& option(const std::string& name, Option&& opt);

  std::optional<std::string> finalizeParsing();

  std::unordered_map<std::string, Option> m_opts;
  std::unordered_map<char, std::string> m_short_names;
};

}
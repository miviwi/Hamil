#include <util/opts.h>
#include <util/format.h>

#include <cassert>
#include <algorithm>
#include <sstream>
#include <regex>
#include <utility>
#include <string_view>

namespace util {

Option::Option(Type type, const std::string& doc, Flags flags) :
  m_flags(flags), m_type(type), m_doc(doc)
{
}

bool Option::operator==(const Option& other) const
{
  return m_val == other.m_val;
}

Option::Type Option::type() const
{
  return (Type)m_type;
}

Option::Flags Option::flags() const
{
  return m_flags;
}

const std::string& Option::doc() const
{
  return m_doc;
}

Option::operator bool() const
{
  return m_val.index() != Empty;
}

bool Option::b() const
{
  if(m_type != Bool) throw TypeError();

  return std::get<bool>(m_val);
}

long Option::i() const
{
  if(m_type != Int) throw TypeError();

  return std::get<long>(m_val);
}

const std::string& Option::str() const
{
  if(m_type != String) throw TypeError();

  return std::get<std::string>(m_val);
}

ConsoleOpts& ConsoleOpts::boolean(const std::string& name, const std::string& doc, Option::Flags flags)
{
  return option(name, Option(Option::Bool, doc, flags));
}

ConsoleOpts& ConsoleOpts::integer(const std::string& name, const std::string& doc, Option::Flags flags)
{
  return option(name, Option(Option::Int, doc, flags));
}

ConsoleOpts& ConsoleOpts::string(const std::string& name, const std::string& doc, Option::Flags flags)
{
  return option(name, Option(Option::String, doc, flags));
}

static const std::regex p_opt_regex("^(?:(-[a-z])|(--[a-z][a-z0-9-]*))(?:=(.+))?$", std::regex::optimize);
std::optional<std::string> ConsoleOpts::parse(int argc, char *argv[])
{
  for(int i = 1; i < argc; i++) {
    std::string_view arg(argv[i]);

    // matches:
    //   [0] = whole pattern
    //   [1] = short-name
    //   [2] = long-name
    //   [3] = value
    std::cmatch matches;
    if(!std::regex_match(arg.data(), matches, p_opt_regex)) throw ParseError(arg.data());

    // need to resolve the name
    std::string name;

    if(matches[1].matched) {    // matched a short-name
      auto ch = *(matches[1].first + 1);

      auto it = m_short_names.find(ch);
      if(it == m_short_names.end()) throw InvalidOptionError(matches[1].str());

      name = it->second;
    } else {                   // matched a long-name
      name = std::string(matches[2].first + 2, matches[2].second);
    }

    auto it = m_opts.find(name);
    if(it == m_opts.end()) throw InvalidOptionError(name);

    auto& opt = it->second;

    // opt already has a value
    if(opt) throw ParseError("option '" + name + "' specified more than once");

    // tries to advance to the next argument in argv[]
    //   returns false if there are no more arguments
    auto next_arg = [&]() -> bool {
      if(i+1 >= argc) return false;

      i++;
      return true;
    };

    switch(opt.type()) {
    case Option::Bool:
      if(matches[3].matched) throw ParseError("boolean '" + name + "' given a value!");

      opt = true;
      break;

    case Option::Int:
      if(matches[3].matched) { // the string after '=' is the value
        char *end = nullptr;
        auto val = std::strtol(matches[3].first, &end, 0);

        // matches[3].second is the end of the whole string
        if(end != matches[3].second) return name;

        opt = val;
      } else {                 // the next arg is the value
        if(!next_arg()) throw MissingValueError(name);

        char *end = nullptr;
        auto val = std::strtol(argv[i], &end, 0);
        
        if(end != argv[i]+strlen(argv[i])) return name;

        opt = val;
      }
      break;

    case Option::String:
      if(matches[3].matched) {
        opt = matches[3].str();
      } else {
        if(!next_arg()) throw MissingValueError(name);

        opt = std::string(argv[i]);
      }
      break;
    }
  }

  return finalizeParsing();
}

std::optional<std::string> ConsoleOpts::finalizeParsing()
{
  // check if all the Required options have values and set all
  // booleans without a value to false
  for(auto& pair : m_opts) {
    auto& opt = pair.second;

    if(opt.type() == Option::Bool) {
      if(!opt) opt = false; // if opt has no value - set it to false
    }

    if(opt.flags() & Option::Required) {
      if(!opt) return pair.first; // a required option has no value - bail out
    }
  }

  return std::nullopt;
}

const Option *ConsoleOpts::get(const std::string& name)
{
  auto it = m_opts.find(name);
  return it != m_opts.end() ? &it->second : nullptr;
}

void ConsoleOpts::foreach(IterFn fn) const
{
  for(const auto& pair : m_opts) {
    const auto& opt = pair.second;

    // if the option has a value - call 'fn'
    if(opt) fn(pair.first, opt);
  }
}

static const std::string p_margin(ConsoleOpts::DocNameMargin, ' ');
std::string ConsoleOpts::doc() const
{
  std::string result;
  result.reserve(256);

  for(const auto& opt : m_opts) {
    std::ostringstream ss;

    ss << "--" << opt.first;
    switch(opt.second.type()) {
    case Option::Bool:   break;
    case Option::Int:    ss << "=<number>"; break;
    case Option::String: ss << "=<string>"; break;

    default: assert(0);
    }

    if(opt.second.flags() & Option::ShortName) ss << ", -" << opt.first.front();

    // algin option names to DocNameMargin columns
    ss << std::string(std::max(DocNameMargin-ss.tellp(), 1LL), ' ');

    util::linewrap(opt.second.doc(), 80-DocNameMargin, [&](const std::string& str, size_t line_no) {
      // if this isn't the first line add the margin
      if(line_no) ss << p_margin;

      ss << str << "\n";
    });

    result += ss.str();
  }

  return result;
}

static const std::regex p_opt_name_regex("^[a-z][a-z0-9-]*$", std::regex::optimize);
ConsoleOpts& ConsoleOpts::option(const std::string& name, Option&& opt)
{
  if(!std::regex_match(name, p_opt_name_regex)) throw Option::NameError(name);

  if(opt.flags() & Option::Required) {
    if(opt.type() == Option::Bool) throw Option::FlagError(
      "the Option::Required flag cannot be set on option '" + name + "' because it has type Option::Bool");
  }

  if(opt.flags() & Option::ShortName) {
    // check it the short name isn't already in use
    auto result = m_short_names.emplace(name.front(), name);

    if(!result.second) throw Option::ShortNameAmbiguousError(name);
  }

  m_opts.emplace(name, std::move(opt));
  return *this;
}

}
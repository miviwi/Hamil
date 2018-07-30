#include <util/str.h>

#include <sstream>
#include <algorithm>

#include <cctype>

namespace util {

void split(const std::string& str, char delim, std::function<void(const std::string& line)> callback)
{
  std::istringstream ss(str);
  std::string line;

  while(std::getline(ss, line, delim)) {
    callback(line);
  }
}

void splitlines(const std::string& str, std::function<void(const std::string& line)> callback)
{
  split(str, '\n', callback);
}

static const std::string p_whitespace = " \t\r\n";
std::string strip(const std::string& str)
{
  auto begin = str.find_first_not_of(p_whitespace);
  if(begin == std::string::npos) return ""; // The string is all whitespace

  auto end = str.find_last_not_of(p_whitespace);
  if(end == std::string::npos) return str.substr(begin); // No trailing whitespace

  return str.substr(begin, end-begin + 1);
}

}
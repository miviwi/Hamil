#pragma once

#include <string>
#include <functional>

namespace util {

std::string fmt(const char *fmt, ...);

// calls 'callback' for every wrapped substring of 'line'
//   with the current (0-based) line number as 'line_no'
void linewrap(const std::string& line, size_t wrap_limit,
  std::function<void(const std::string& line, size_t line_no)> callback);

}
#pragma once

#include <common.h>

#include <string>
#include <functional>

namespace util {

void split(const std::string& str, char delim, std::function<void(const std::string& line)> callback);

// calls 'callback' for each line in 'str'
void splitlines(const std::string& str, std::function<void(const std::string& line)> callback);

// strips leading and trailing shitespace
std::string strip(const std::string& str);

}
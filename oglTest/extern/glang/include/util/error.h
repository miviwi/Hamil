#pragma once

#include <string>

namespace glang {

std::string error_location(const char *name, unsigned line, unsigned column, const char *source);

}
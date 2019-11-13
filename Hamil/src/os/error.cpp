#include <os/error.h>

#include <string>

#include <cassert>

namespace os {

const char *Error::what() const noexcept
{
  if(std::holds_alternative<std::string_view>(m_error)) {
    return std::get<std::string_view>(m_error).data();
  } else if(std::holds_alternative<unsigned>(m_error)) {
    return "";
  }

  assert(0);   // Unreachable

  return nullptr;
}

}

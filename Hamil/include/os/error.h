#pragma once

#include <common.h>

#include <exception>
#include <string_view>
#include <variant>

namespace os {

struct Error : public std::exception {
  Error(unsigned err) : m_error(err) { }
  Error(std::string_view err) : m_error(err) { }

  Error() : m_error("a runtime error has occured!") { }

  virtual const char *what() const noexcept final;

private:
  std::variant<std::string_view, unsigned> m_error;
};

}

#include <os/clipboard.h>

namespace os {

void Clipboard::string(const std::string& str)
{
  string(str.data(), str.size());
}

}

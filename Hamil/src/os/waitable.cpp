#include <os/waitable.h>

namespace os {

Waitable::~Waitable()
{
  deref();
}

}

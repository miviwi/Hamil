#include <sysv/inputman.h>

namespace sysv {

InputDeviceManager::InputDeviceManager()
{
}

Input::Ptr InputDeviceManager::doPollInput()
{
  Input::Ptr input(nullptr, &Input::deleter);

#if __sysv
#endif

  return input;
}


}

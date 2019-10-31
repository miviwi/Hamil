#include <win32/handle.h>

#if defined(_MSVC_VER)
#  include <Windows.h>
#endif

namespace win32 {

Handle::Handle() :
  m(nullptr)
{
}

Handle::Handle(void *handle) :
  m(handle)
{
}

Handle::Handle(Handle&& other) :
  m(other.m)
{
  other.m = nullptr;
}

Handle::~Handle()
{
  if(deref()) return;

#if defined(_MSVC_VER)
  if(m && m != INVALID_HANDLE_VALUE) CloseHandle(m);
#endif
}

Handle& Handle::operator=(Handle&& other)
{
  if(m || refs() > 1) this->~Handle();

  m = other.m;
  other.m = nullptr;

  return *this;
}

void *Handle::handle() const
{
  return m;
}

}

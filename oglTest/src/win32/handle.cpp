#include <win32/handle.h>

#include <Windows.h>

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

  if(m && m != INVALID_HANDLE_VALUE) CloseHandle(m);
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
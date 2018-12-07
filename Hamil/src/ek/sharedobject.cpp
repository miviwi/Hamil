#include <ek/sharedobject.h>

namespace ek {

SharedObject::SharedObject() :
  m_in_use(false)
{
}

SharedObject::SharedObject(SharedObject&& other) :
  m_in_use(other.m_in_use.load())
{
  other.m_in_use.store(false);
}

bool SharedObject::lock() const
{
  bool locked = false;

  return m_in_use.compare_exchange_strong(locked, true);
}

void SharedObject::unlock() const
{
  m_in_use.store(false);
}

}
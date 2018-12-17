#include <ek/sharedobject.h>

namespace ek {

SharedObject::SharedObject() :
  m_in_use(false),
  m_fence(std::nullopt)
{
}

SharedObject::SharedObject(SharedObject&& other) :
  m_in_use(other.m_in_use.load()),
  m_fence(std::move(other.m_fence))
{
  other.m_in_use.store(false);
}

bool SharedObject::lock(const gx::Fence& fence) const
{
  if(!lock()) return false;

  // We've acquired the lock, set the Fence
  m_fence.emplace(fence);
  return true;
}

bool SharedObject::lock() const
{
  if(m_fence) {    // If a Fence was assigned, check if first
    if(!m_fence->signaled()) return false;
  }

  m_fence = std::nullopt;

  // ...and when it's signaled, check the lock
  bool locked = false;
  return m_in_use.compare_exchange_strong(locked, true);
}

void SharedObject::unlock() const
{
  m_in_use.store(false);
}

}
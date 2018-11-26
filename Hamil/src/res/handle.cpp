#include <res/handle.h>
#include <res/manager.h>

namespace res {

HandleBase::HandleBase(Resource::Id id) :
  m(resources().handle(id))
{
}

Resource::Ptr& HandleBase::lock()
{
  return m_locked ? m_locked : m_locked = m.lock();
}

}
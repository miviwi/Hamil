#include <hm/components/gameobject.h>

namespace hm {

const std::string& GameObject::name() const
{
  return m_name;
}

void GameObject::destroyed()
{
}

}
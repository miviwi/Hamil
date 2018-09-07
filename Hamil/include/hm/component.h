#pragma once

#include <hm/hamil.h>
#include <hm/entity.h>

#include <util/hashindex.h>

#include <array>
#include <vector>

namespace hm {

class Component {
public:
  Component(Entity e);
  
  Entity entity() const;

  // Only returns 'true' when it's Entity is != Invalid and alive()
  operator bool() const;

private:
  friend class IComponentStore;

  Entity m_entity;
};

}
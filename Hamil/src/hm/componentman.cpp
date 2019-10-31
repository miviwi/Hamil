#include <hm/componentman.h>
#include <hm/componentstore.h>

#include <components.h>
#include <hm/components/all.h>

#include <utility>

namespace hm {

// The concrete class definition must be placed here, otherwise
//   there is a circular dependency ComponentManager -> ComponentStore
class ComponentManager : public IComponentManager {
public:
  enum {
    InitialHashSize       = 1024,
    InitialComponentsSize = 512,
  };

  ComponentManager()
  {
    m_components = create_component_store<ComponentStore>(
      InitialHashSize, InitialComponentsSize
    );
  }

  ~ComponentManager()
  {
    delete m_components;
  }

protected:

  virtual ComponentStore& components()
  {
    return *m_components;
  }

private:
  ComponentStore *m_components;
};

IComponentManager::~IComponentManager()
{
}

IComponentManager::Ptr create_component_manager()
{
  IComponentManager::Ptr ptr;
  ptr.reset(new ComponentManager());

  return ptr;
}

IComponentManager& IComponentManager::lock()
{
  components().lock();

  return *this;
}

IComponentManager& IComponentManager::unlock()
{
  components().unlock();

  return *this;
}

IComponentManager& IComponentManager::requireUnlocked()
{
  components().requireUnlocked();

  return *this;
}

IComponentManager& IComponentManager::endRequireUnlocked()
{
  components().endRequireUnlocked();

  return *this;
}

}

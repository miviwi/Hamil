#include <hm/componentmeta.h>

#include <hm/components/all.h>

namespace hm {

template <>
class ComponentMetaclass<GameObject> : public IComponentMetaclass {
public:
  ComponentMetaclass()
  {
    m_static = {
      .protoid  = ComponentProto::GameObject,
      .data_size = sizeof(hm::GameObject),
      .flags     = 0
    };
  }

  virtual const ComponentMetaclassStatic& staticData() const { return m_static; }

protected:
  ComponentMetaclassStatic m_static;
};

}

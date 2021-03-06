#pragma once

#include <hm/hamil.h>

#include <util/abstracttuple.h>

#include <type_traits>
#include <tuple>

namespace hm {

// Forward declarations
class Component;

struct ComponentMetaclassStatic {
  enum Flags : u32 {
    IsTagComponent = (1<<0),    // Components with this flag set have no associated
                                //   data (data_size == 0) and only serve to organize
                                //   related Entities into the same PrototypeChunk
  };

  ComponentProtoId protoid = (ComponentProtoId)~0u;   // ComponentProto::Invalid
  size_t data_size = 0;         // sizeof(Component)
  u32 flags = 0;                // bitwise OR of 'Flags' values
};

class IComponentMetaclass {
public:
  using StaticData = ComponentMetaclassStatic;

  virtual ~IComponentMetaclass() = default;

  // Returns the ComponentMetaclassStatic struct which describes the Component
  virtual const StaticData& staticData() const = 0;

  //virtual Component *ctor(Component *self, const util::IAbstractTuple *args) const = 0;
  
private:
};

template <typename Component>
class ComponentMetaclassBase : public IComponentMetaclass {
public:
  // Disallow copy- and move-construction (ComponentMetaclasses
  //   can be accessed only via pointers returned by
  //   metaclass_from_type()/metaclass_from_protoid())
  ComponentMetaclassBase(const ComponentMetaclassBase&) = delete;
  ComponentMetaclassBase(ComponentMetaclassBase&&) = delete;

  virtual ~ComponentMetaclassBase() = default;

  virtual const StaticData& staticData() const final { return m_metaclass_static; }

protected:
  ComponentMetaclassBase() = default;   // Prevent instantiating this class directly

  // MUST be initialized in the derived class' constructor
  StaticData m_metaclass_static;
};

// Fully explicitly specialized class template (specializations
//   declared in <components.h> generated by Eugene.componentgen)
template <typename Component>
class ComponentMetaclass;

// Specialization declarations and definitions generated by Eugene.componentgen
template <typename T>
inline IComponentMetaclass *metaclass_from_type();

// Definition generated by Eugene.componentgen
//   TODO: currently all the ComponentMetaclasses are stored
//      in an array of std::unique_ptr<IComponentMetaclass>,
//      which is bad for cache coherence because they could
//      be spread randomly across memory
//   - Manually allocate as many blocks as there are Metaclasses
//     in a contiguous array all sized to fit the largest Metaclass,
//     which will make fetching them more cache-friendly
IComponentMetaclass *metaclass_from_protoid(ComponentProtoId protoid);

}

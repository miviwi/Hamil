#pragma once

#include <common.h>

#include <util/staticstring.h>

namespace hm {

// Forward declarations
struct ComponentProto;

class IComponentManager;
class IEntityManager;
// --------------------

// Type wide enough to store any ComponentProto::Id bit index
using ComponentProtoId = u32;

using ComponentTag = util::StaticString;

// Globally unique non-repeating Entity identifier/handle
using EntityId = u32;

// Determines the width of the util::FixedBitVector used to store
//   a hm::Prototype's list of Components
static constexpr unsigned NumComponentProtoIdBits = 128;

void init();
void finalize();

IComponentManager& components();
IEntityManager& entities();

void frame();

}

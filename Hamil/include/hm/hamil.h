#pragma once

#include <common.h>

#include <util/staticstring.h>

namespace hm {

// Forward declarations
struct ComponentProto;

class World;
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

// Returns a reference to a freshly-allocated World, allocated during hm::init()
//   - This World had ONLY alloc() called and NOT createEmpty() (or another
//     method to initialize it's internal structures).
//   XXX: This gives the flexibility to pick an initialization method depending
//      on context (ex. from serialized data when loading a save file)
World& world();

// TODO: was this intended to do anything..?
//   get rid of it if not :)
void frame();

}

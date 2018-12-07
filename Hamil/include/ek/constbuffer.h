#pragma once

#include <ek/euklid.h>
#include <ek/sharedobject.h>

#include <util/ref.h>

#include <atomic>

namespace gx {
class ResourcePool;
class UniformBuffer;
}

namespace ek {

class Renderer;

class ConstantBuffer : public SharedObject {
public:
  ConstantBuffer(u32 id, size_t sz);
  ConstantBuffer(ConstantBuffer&& other);

  u32 id() const;
  gx::UniformBuffer& get(gx::ResourcePool& pool) const;

  size_t size() const;

private:
  u32 m_id;
  size_t m_sz;
};

}
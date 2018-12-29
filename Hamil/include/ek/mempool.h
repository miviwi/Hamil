#pragma once

#include <ek/sharedobject.h>

namespace gx {
class MemoryPool;
}

namespace ek {

class MemoryPool : public SharedObject {
public:
  MemoryPool(size_t size);

  gx::MemoryPool& get();
  const gx::MemoryPool& get() const;

  gx::MemoryPool *ptr();

private:
  gx::MemoryPool *m_pool;
};

}
#pragma once

#include <ek/euklid.h>

#include <util/ref.h>
#include <gx/fence.h>

#include <atomic>
#include <optional>

namespace ek {

class Renderer;

class SharedObject {
public:
  SharedObject();
  SharedObject(SharedObject&& other);

protected:
  friend Renderer;

  // Returns 'true' if the SharedObject was locked successfully
  //   i.e. wasn't already in use before the call and it's
  //   fence is in the Fence::Signaled state
  bool lock(const gx::Fence& fence) const;
  // Acts the same as lock(gx::Fence&) except after the lock
  //   is successfully acquired no fence is assigned to
  //   guard the object
  bool lock() const;
  // Marks the SharedObject as no longer in use
  void unlock() const;

  mutable std::atomic<bool> m_in_use;
  mutable std::optional<gx::Fence> m_fence;
};

}

#pragma once

#include <ek/euklid.h>

#include <util/ref.h>

#include <atomic>

namespace ek {

class Renderer;

class SharedObject {
public:
  SharedObject();
  SharedObject(SharedObject&& other);

protected:
  friend Renderer;

  // Returns 'true' if the SharedObject was locked successfully
  //   i.e. wasn't already in use before the call
  bool lock() const;
  // Marks the SharedObject as no longer in use
  void unlock() const;

  mutable std::atomic<bool> m_in_use;
};

}

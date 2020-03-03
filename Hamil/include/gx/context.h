#pragma once

#include <gx/gx.h>
#include <os/error.h>
#include <util/ref.h>

#include <memory>

namespace os {
// Forward declaration
class Window;
}

namespace gx {

// Forward declarations
class TexImageUnit;
class BufferBindPoint;
class Texture2D;
class Sampler;

enum BufferBindPointType : unsigned;
// --------------------

struct GLVersion {
  int major, minor;
};

class GLContext : public Ref {
public:
  struct AcquireError final : public os::Error {
    AcquireError() :
      os::Error("failed to acquire the GLContext!")
    { }
  };

  struct NotADebugContextError final : public os::Error {
    NotADebugContextError() :
      os::Error("the operation can only be performed on a debug OpenGL context!")
    { }
  };

  GLContext();
  GLContext(const GLContext& other) = delete;
  virtual ~GLContext();

  GLContext& operator=(const GLContext& other) = delete;

  // Returns a reference to the GLContext on which
  //   makeCurrent() was last called on this thread
  static GLContext& current();

  // Returns the OS-native hande to the GLContext
  virtual void *nativeHandle() const = 0;

  virtual GLContext& acquire(os::Window *window, GLContext *share = nullptr) = 0;

  // Once called for the first time on a given Thread
  //   all future calls to this method must be made on
  //   that Thread
  GLContext& makeCurrent();

  // Frees the GLContext
  //   - Handles 'null' GLContexts by acting as a no-op
  GLContext& release();
  
  // Returns 'true' if context was previously acquire()'d
  operator bool() const;

  TexImageUnit& texImageUnit(unsigned slot);

  BufferBindPoint& bufferBindPoint(
      BufferBindPointType bind_point, unsigned index
  );

  // Can only be called AFTER gx::init()!
  GLContext& dbg_EnableMessages();

  GLContext& dbg_PushCallGroup(const char *name);
  GLContext& dbg_PopCallGroup();

  std::string versionString();
  GLVersion version();

protected:
  virtual bool wasInit() const = 0;

  virtual void doMakeCurrent() = 0;
  virtual void doRelease() = 0;

  // MUST be called by derived makeCurrent() right before they return!
  void postMakeCurrentHook();

private:
  void cleanupInternal();

  TexImageUnit *m_tex_image_units;
  BufferBindPoint *m_buffer_binding_points;

  unsigned m_dbg_group_id;
};

}

#pragma once

#include <common.h>

#include <ui/uicommon.h>
#include <ui/cursor.h>
#include <ui/event.h>
#include <ui/style.h>
#include <ui/drawable.h>

#include <math/geometry.h>
#include <win32/input.h>
#include <win32/window.h>
#include <gx/buffer.h>
#include <gx/vertex.h>
#include <gx/resourcepool.h>
#include <gx/memorypool.h>

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <utility>
#include <type_traits>

namespace gx {
class CommandBuffer;
}

namespace ui {

// Must be called AFTER gx::init()!
void init();
void finalize();

class VertexPainter;
class Frame;

using InputPtr = win32::Input::Ptr;

class Ui {
public:
  Ui(gx::ResourcePool& pool, Geometry geom, const Style& style);
  Ui(const Ui& other) = delete;
  ~Ui();

  Ui& operator=(const Ui& other) = delete;

  // Intended for internal use
  ivec4 scissorRect(Geometry g);

  // When not provided a 1:1 mapping from
  //   Framebuffer to Window pixels is assumed
  Ui& realSize(vec2 real_size);

  // Makes 'frame' a child of the Ui, which means it will:
  //   * react to input()
  //   * get drawn by paint()
  //   * be automatically deallocated by the Ui
  Ui& frame(Frame *frame, vec2 pos);
  // Identical to frame(Frame *, vec2) except it keeps
  //   the Frame's current position
  Ui& frame(Frame *frame);

  // The 'frame' MUST be heap-allocated, this overload is
  //   only provided for convenience when using create()
  Ui& frame(Frame& frame, vec2 pos);
  // See frame(Frame *)
  Ui& frame(Frame& frame);

  template <typename T, typename... Args>
  Ui& frame(Args&&... args)
  {
    Frame *f = new T(std::forward<Args>(args)...);
    return frame(f);
  }

  // Can be used to change/specify the name of a Frame
  //   after it's construction
  // Sets the Frame's 'm_name' pointer to an internalized
  //   version of the string
  void registerFrame(Frame *frame);

  // Returns a pointer to a Frame named 'name' or
  //   nullptr if no such Frame exists
  Frame *getFrameByName(const std::string& name);

  // Convenience method which casts the return value
  //   of getFrameByName() to the specified type
  template <typename T>
  T *getFrameByName(const std::string& name)
  {
    static_assert(std::is_base_of_v<Frame, T>, "T is not a Frame!");

    return (T *)getFrameByName(name);
  }

  // Used by Frames
  const Style& style() const;

  // The DrawableManager associated with this Ui
  DrawableManager& drawable();

  // The gx::CommandBuffer returned from paint()
  //   draws into this texture
  //  - This texture is of type Texture2D(Multisample)
  gx::ResourcePool::Id /* Texture2D(Multisample) */ framebufferTextureId();

  // Call this directly with input from win32::Window::getInput()
  //   - When the return value == true it means the input
  //     was processed by the Ui and should be discarded,
  //     otherwise (when the return value == false) the input
  //     didn't touch any Frames and should be further
  //     processed by the application
  bool input(CursorDriver& cursor, const InputPtr& input);
  // Generates a gx::CommandBuffer which will draw the Ui
  //   into the texture referenced by framebufferTextureId()
  gx::CommandBuffer paint();

  // For internal use (DO NOT call externally)
  void capture(Frame *frame);
  // For internal use (DO NOT call externally)
  void keyboard(Frame *frame);

private:
  // Size of the Framebuffer the Ui will be drawn to
  //   - Needed to correctly specify gx::Pipeline::scissor()
  //     parameters
  vec2 m_real_size;

  // No input is consumed outside of this Geometry and
  //   nothing is drawn
  Geometry m_geom;

  // Default style for all added Frames
  Style m_style;

  // List of all Frames which are direct children of this Ui
  std::vector<Frame *> m_frames;
  // Lookup cache for getFrameByName()
  std::unordered_map<std::string, Frame *> m_names;

  // Pointer to the Frame which currently holds mouse focus
  Frame *m_capture;
  // Pointer to the Frame which currently hold keyboard focus
  Frame *m_keyboard;

  // External ResourcePool provided in the constructor,
  //   shared by the DrawableManager
  //  - The ResourcePool is allocated externally so the Ui's
  //    Framebuffer Texture can be accessed easily
  gx::ResourcePool& m_pool;
  // Private MemoryPool
  gx::MemoryPool m_mempool;

  gx::ResourcePool::Id /* Texture2D(Multisample) */ m_framebuffer_tex_id;
  gx::ResourcePool::Id m_framebuffer_id;
  gx::ResourcePool::Id m_program_id;
  gx::ResourcePool::Id m_renderpass_id;

  // Returned by drawable()
  //   - Caches text to be painted on Labels, Buttons, DropDowns etc
  //   - Stores (and manages) all the images used within this Ui
  DrawableManager m_drawable;

  // Used as argument for all Frame::paint() calls
  VertexPainter m_painter;

  // Set to 'true' when frame vertices need to be repainted (TODO)
  bool m_repaint;

  // 'm_vtx_id' backing buffer, invalidated everytime paint() is called
  gx::VertexBuffer m_buf;
  gx::IndexBuffer m_ind;

  // Source for paint() draw call vertices
  //   - See VertexPainter::Fmt for the format of the vertices
  gx::ResourcePool::Id m_vtx_id;
};

}
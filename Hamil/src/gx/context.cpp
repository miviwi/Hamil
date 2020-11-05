#include <gx/gx.h>
#include <gx/context.h>
#include <gx/texture.h>
#include <gx/buffer.h>

#include <os/window.h>

#include <cassert>
#include <cstdlib>

#include <new>

namespace gx {

static void ogl_debug_callback(GLenum source, GLenum type, GLuint id,
                               GLenum severity, GLsizei length, const GLchar *msg, const void *user);

thread_local static GLContext *p_current = nullptr;

GLContext::GLContext() :
  m_tex_image_units(nullptr),
  m_buffer_binding_points(nullptr),
  m_dbg_group_id(1)
{
  // Allocate backing memory via malloc() because GLTexImageUnit's constructor requires
  //   an argument and new[] doesn't support passing per-instance args
  m_tex_image_units = (TexImageUnit *)malloc(NumTexUnits * sizeof(TexImageUnit));
  for(unsigned i = 0; i < NumTexUnits; i++) {
    auto unit = m_tex_image_units + i;

    // Use placement-new to finalize creation of the object
    new(unit) TexImageUnit(this, i);
  }

  // Do the same for all the GLBufferBindPoints
  m_buffer_binding_points = (BufferBindPoint *)malloc(
      NumUniformBindings * BufferBindPointType::NumTypes * sizeof(BufferBindPoint)
  );
  for(size_t type_ = 0; type_ < BufferBindPointType::NumTypes; type_++) {
    auto type = (BufferBindPointType)type_;
    auto bind_points = m_buffer_binding_points + type_*NumUniformBindings;

    for(unsigned i = 0; i < NumUniformBindings; i++) {
      auto bind_point = bind_points + i;

      new(bind_point) BufferBindPoint(this, type, i);
    }
  }
}

GLContext::~GLContext()
{
  if(deref()) return;

  // Make sure not to leave dangling pointers around
  if(p_current == this) p_current = nullptr;
}

GLContext& GLContext::current()
{
  assert(p_current &&
      "attempted to get the current() GLContext without a previous call to GLContext::makeCurrent()");

  return *p_current;
}

GLContext& GLContext::makeCurrent()
{
  if(p_current) p_current->deref();

  doMakeCurrent();

  p_current = this;
  ref();    // We're storing an additional reference

  return *this;
}

GLContext& GLContext::release()
{
  if(!wasInit()) return *this;    // release() was already called or this context
                                  //   has never been acquire()'d

  // Make sure a context can never be considered
  //   'current' once release() has been called
  if(p_current == this) {
    deref();
    p_current = nullptr;
  }

  doRelease();
  cleanupInternal();

  return *this;
}

GLContext::operator bool() const
{
  return wasInit();
}

TexImageUnit& GLContext::texImageUnit(unsigned slot)
{
  return m_tex_image_units[slot];
}

BufferBindPoint& GLContext::bufferBindPoint(BufferBindPointType bind_point, unsigned index)
{
  return m_buffer_binding_points[bind_point*BufferBindPointType::NumTypes + index];
}

GLContext& GLContext::dbg_EnableMessages()
{
#if !defined(NDEBUG)
  int context_flags = -1;
  glGetIntegerv(GL_CONTEXT_FLAGS, &context_flags);

  if(!(context_flags & GL_CONTEXT_FLAG_DEBUG_BIT)) throw NotADebugContextError();

  // Enable debug output
  glEnable(GL_DEBUG_OUTPUT);
  glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

  glDebugMessageCallback(ogl_debug_callback, nullptr);
#endif

  return *this;
}

GLContext& GLContext::dbg_PushCallGroup(const char *name)
{
#if !defined(NDEBUG)
  glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, m_dbg_group_id, -1, name);
  m_dbg_group_id++;
#endif

  return *this;
}

GLContext& GLContext::dbg_PopCallGroup()
{
#if !defined(NDEBUG)
  glPopDebugGroup();
#endif

  return *this;
}

std::string GLContext::versionString()
{
  return std::string((const char *)glGetString(GL_VERSION));
}

GLVersion GLContext::version()
{
  auto version_string = glGetString(GL_VERSION);

  GLVersion version;
  sscanf((const char *)version_string, "%d.%d", &version.major, &version.minor);

  return version;
}

void GLContext::cleanupInternal()
{
  // Because malloc() was used to allocate 'm_tex_image_units' we need to call
  //   the destructors on each of the GLTexImageUnits manually...
  for(unsigned i = 0; i < NumTexUnits; i++) {
    auto unit = m_tex_image_units + i;
    unit->~TexImageUnit();
  }

  // ...and release the memory with free()
  free(m_tex_image_units);

  // ...and do the same for all the GLBufferBindPoints
  for(unsigned i = 0; i < NumUniformBindings*BufferBindPointType::NumTypes; i++) {
    auto bind_point = m_buffer_binding_points + i;
    bind_point->~BufferBindPoint();
  }

  free(m_buffer_binding_points);
}

static void ogl_debug_callback(GLenum source, GLenum type, GLuint id,
                               GLenum severity, GLsizei length, const GLchar *msg, const void *user)
{
  const char *source_str = "";
  switch(source) {
  case GL_DEBUG_SOURCE_API:             source_str = "GL_API"; break;
  case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   source_str = "GL_WINDOW_SYSTEM"; break;
  case GL_DEBUG_SOURCE_SHADER_COMPILER: source_str = "GL_SHADER_COMPILER"; break;
  case GL_DEBUG_SOURCE_APPLICATION:     source_str = "GL_APPLICATION"; break;
  case GL_DEBUG_SOURCE_THIRD_PARTY:     source_str = "GL_THIRD_PARTY"; break;
  case GL_DEBUG_SOURCE_OTHER:           source_str = "GL_OTHER"; break;
  }

  const char *type_str = "";
  switch(type) {
  case GL_DEBUG_TYPE_ERROR:               type_str = "error!"; break;
  case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: type_str = "deprecated"; break;
  case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  type_str = "undefined!"; break;
  case GL_DEBUG_TYPE_PORTABILITY:         type_str = "portability"; break;
  case GL_DEBUG_TYPE_PERFORMANCE:         type_str = "performance"; break;
  case GL_DEBUG_TYPE_OTHER:               type_str = "other"; break;
  }

  const char *severity_str = "";
  switch(severity) {
  case GL_DEBUG_SEVERITY_HIGH:         severity_str = "!!!"; break;
  case GL_DEBUG_SEVERITY_MEDIUM:       severity_str = "!!"; break;
  case GL_DEBUG_SEVERITY_LOW:          severity_str = "!"; break;
  case GL_DEBUG_SEVERITY_NOTIFICATION: severity_str = "?"; break;
  }

#if 0
  printf("%s (%s, %s): %s\n", source_str, severity_str, type_str, msg);
#endif
}

}

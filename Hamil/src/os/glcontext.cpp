#include <os/glcontext.h>
#include <gx/context.h>

#include <config>

#include <win32/window.h>
#include <win32/glcontext.h>

#include <sysv/window.h>
#include <sysv/glcontext.h>

namespace os {

std::unique_ptr<gx::GLContext> create_glcontext()
{
  gx::GLContext *gl_context = nullptr;

#if __win32
  gl_context = new win32::GLContext();
#elif __sysv
  gl_context = new sysv::GLContext();
#else
#  error "unknown platform"
#endif

  return std::unique_ptr<gx::GLContext>(gl_context);
}

}

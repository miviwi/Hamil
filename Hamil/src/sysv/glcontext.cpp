#include <sysv/glcontext.h>
#include <sysv/window.h>

#include <os/window.h>

#include <config>

#include <cassert>

namespace sysv {

GLContext::GLContext() :
  gx::GLContext()
{
}

GLContext::~GLContext()
{
  release();
}

void *GLContext::nativeHandle() const
{
  return nullptr;
}

gx::GLContext& GLContext::acquire(os::Window *window_, gx::GLContext *share)
{
  auto window = (sysv::Window *)window_;

#if __sysv
#endif

  return *this;
}

void GLContext::doMakeCurrent()
{
#if __sysv
#endif
}

void GLContext::doRelease()
{
#if __sysv
#endif
}

bool GLContext::wasInit() const
{
  return false;
}

}

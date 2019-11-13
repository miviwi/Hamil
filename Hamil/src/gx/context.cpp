#include <gx/context.h>

#include <os/window.h>

#include <cassert>

namespace gx {

thread_local static GLContext *p_current = nullptr;

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

  return *this;
}

GLContext::operator bool() const
{
  return wasInit();
}

}

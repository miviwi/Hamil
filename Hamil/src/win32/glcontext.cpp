#include <win32/glcontext.h>

#include <Windows.h>

#include <GL/gl3w.h>
#include <GL/wgl.h>

#include <cassert>
#include <utility>

namespace win32 {

OGLContext::OGLContext() :
  m_hdc(nullptr),
  m_hglrc(nullptr)
{
}

OGLContext::OGLContext(OGLContext&& other) :
  m_hdc(other.m_hdc),
  m_hglrc(other.m_hglrc)
{
#if !defined(NDEBUG)
  m_owner = other.m_owner;
  other.m_owner = win32::Thread::InvalidId;

  assert((m_owner == win32::Thread::InvalidId || Thread::current_thread_id() == m_owner) &&
    "Attempted to move an OGLContext onto a different thread!");
#endif

  other.m_hdc = other.m_hglrc = nullptr;
}

OGLContext& OGLContext::operator=(OGLContext&& other)
{
  if(*this) release();

  new(this) OGLContext(std::move(other));
  return *this;
}

void OGLContext::makeCurrent()
{
#if !defined(NDEBUG)
  auto thread = Thread::current_thread_id();
  if(m_owner != Thread::InvalidId) {
    assert(thread == m_owner &&
      "makeCurrent() must always be called on the same Thread for a given OGLContext!");
  } else {
    m_owner = thread;
  }
#endif

  assert(m_hdc && m_hglrc && "Attempted to use an invalid OGLContext!");

  // wglMakeCurrent() can sometimes fail, when that happens - yield
  //   and try again
  while(wglMakeCurrent((HDC)m_hdc, (HGLRC)m_hglrc) != TRUE) Sleep(1);
}

void OGLContext::release()
{
  wglMakeCurrent(nullptr, nullptr);
  wglDeleteContext((HGLRC)m_hglrc);

  m_hdc = nullptr;
  m_hglrc = nullptr;
}

OGLContext::operator bool()
{
  return m_hdc && m_hglrc;
}

OGLContext::OGLContext(void *hdc, void *hglrc) :
  m_hdc(hdc),
  m_hglrc(hglrc)
{
}

}
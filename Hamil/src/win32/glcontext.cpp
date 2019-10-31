#include <win32/glcontext.h>

#if defined(_MSVC_VER)
#  include <Windows.h>
#endif

#include <GL/gl3w.h>
#if defined(_MSVC_VER)
#  include <GL/wgl.h>
#else
#endif

#include <cassert>
#include <utility>

namespace win32 {

GlContext::GlContext() :
  m_hdc(nullptr),
  m_hglrc(nullptr)
{
}

GlContext::GlContext(GlContext&& other) :
  m_hdc(other.m_hdc),
  m_hglrc(other.m_hglrc)
{
#if !defined(NDEBUG)
  m_owner = other.m_owner;
  other.m_owner = win32::Thread::InvalidId;

  assert((m_owner == win32::Thread::InvalidId || Thread::current_thread_id() == m_owner) &&
    "Attempted to move a GlContext onto a different thread!");
#endif

  other.m_hdc = other.m_hglrc = nullptr;
}

GlContext& GlContext::operator=(GlContext&& other)
{
  if(*this) release();

  new(this) GlContext(std::move(other));
  return *this;
}

void GlContext::makeCurrent()
{
#if !defined(NDEBUG)
  auto thread = Thread::current_thread_id();
  if(m_owner != Thread::InvalidId) {
    assert(thread == m_owner &&
      "makeCurrent() must always be called on the same Thread for a given GlContext!");
  } else {
    m_owner = thread;
  }
#endif

  assert(m_hdc && m_hglrc && "Attempted to use an invalid GlContext!");

#if defined(_MSVC_VER)
  // wglMakeCurrent() can sometimes fail, when that happens - yield
  //   and try again
  while(wglMakeCurrent((HDC)m_hdc, (HGLRC)m_hglrc) != TRUE) Sleep(1);
#endif
}

void GlContext::release()
{
#if defined(_MSVC_VER)
  wglMakeCurrent(nullptr, nullptr);
  wglDeleteContext((HGLRC)m_hglrc);

  m_hdc = nullptr;
  m_hglrc = nullptr;
#endif
}

GlContext::operator bool()
{
  return m_hdc && m_hglrc;
}

GlContext::GlContext(void *hdc, void *hglrc) :
  m_hdc(hdc),
  m_hglrc(hglrc)
{
}

}

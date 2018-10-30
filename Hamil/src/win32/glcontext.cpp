#include <win32/glcontext.h>

#include <Windows.h>

#include <GL/gl3w.h>
#include <GL/wgl.h>

#include <cassert>

namespace win32 {

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

  assert(m_hdc && m_hglrc && "Attempted to use a deleted OGLContext!");

  wglMakeCurrent((HDC)m_hdc, (HGLRC)m_hglrc);
}

void OGLContext::release()
{
  wglMakeCurrent(nullptr, nullptr);
  wglDeleteContext((HGLRC)m_hglrc);

  m_hdc = nullptr;
  m_hglrc = nullptr;
}

OGLContext::OGLContext(void *hdc, void *hglrc) :
  m_hdc(hdc),
  m_hglrc(hglrc)
{
}

}
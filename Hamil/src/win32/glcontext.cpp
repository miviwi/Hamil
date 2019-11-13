#include <win32/glcontext.h>
#include <win32/window.h>

#include <os/window.h>

#include <config>

#if __win32
#  include <Windows.h>
#  include <GL/wgl.h>
#endif

#include <GL/gl3w.h>

#include <cassert>
#include <utility>

namespace win32 {

#if __win32

#if !defined(NDEBUG)
constexpr int ContextFlags = WGL_CONTEXT_DEBUG_BIT_ARB;
#else
constexpr int ContextFlags = 0;
#endif

constexpr int ContextProfile = WGL_CONTEXT_CORE_PROFILE_BIT_ARB;

#endif

static const int ContextAttribs[] = {
#if __win32
  WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
  WGL_CONTEXT_MINOR_VERSION_ARB, 3,
  WGL_CONTEXT_PROFILE_MASK_ARB, ContextProfile,
  WGL_CONTEXT_FLAGS_ARB, ContextFlags,
#endif
  0
};

GLContext::GLContext() :
  gx::GLContext(),
  m_hdc(nullptr),
  m_hglrc(nullptr)
{
}

GLContext::GLContext(GLContext&& other) :
  m_hdc(other.m_hdc),
  m_hglrc(other.m_hglrc)
{
#if !defined(NDEBUG)
  m_owner = other.m_owner;
  other.m_owner = os::Thread::InvalidId;

  assert((m_owner == os::Thread::InvalidId || os::Thread::current_thread_id() == m_owner) &&
    "Attempted to move a GLContext onto a different thread!");
#endif

  other.m_hdc = other.m_hglrc = nullptr;
}

GLContext::~GLContext()
{
  release();
}

GLContext& GLContext::operator=(GLContext&& other)
{
  if(*this) release();

  new(this) GLContext(std::move(other));
  return *this;
}

void *GLContext::nativeHandle() const
{
  return m_hglrc;
}

gx::GLContext& GLContext::acquire(os::Window *window_, gx::GLContext *share)
{
  auto window = (win32::Window *)window_;

#if __win32
  HWND hwnd = (HWND)window->nativeHandle();

  m_hdc = (void *)GetDC(hwnd);
  m_hglrc = CreateContextAttribsARB(
      m_hdc, share ? share->nativeHandle() : nullptr, 
      ContextAttribs
  );
#endif

  return *this;
}

void GLContext::doMakeCurrent()
{
#if !defined(NDEBUG)
  auto thread = Thread::current_thread_id();
  if(m_owner != Thread::InvalidId) {
    assert(thread == m_owner &&
      "makeCurrent() must always be called on the same Thread for a given GLContext!");
  } else {
    m_owner = thread;
  }
#endif

  assert(m_hdc && m_hglrc && "Attempted to use an invalid GLContext!");

#if __win32
  // wglMakeCurrent() can sometimes fail, when that happens - yield
  //   and try again
  while(wglMakeCurrent((HDC)m_hdc, (HGLRC)m_hglrc) != TRUE) Sleep(1);
#endif
}

void GLContext::doRelease()
{
#if __win32
  wglMakeCurrent(nullptr, nullptr);
  wglDeleteContext((HGLRC)m_hglrc);

  m_hdc = nullptr;
  m_hglrc = nullptr;
#endif
}

bool GLContext::wasInit() const
{
  return m_hdc && m_hglrc;
}

}

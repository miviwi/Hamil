#include <win32/glcontext.h>

#include <Windows.h>

#include <GL/gl3w.h>
#include <GL/wgl.h>

namespace win32 {

void OGLContext::makeCurrent()
{
  wglMakeCurrent((HDC)m_hdc, (HGLRC)m_hglrc);
}

OGLContext::OGLContext(void *hdc, void *hglrc) :
  m_hdc(hdc),
  m_hglrc(hglrc)
{
}

}
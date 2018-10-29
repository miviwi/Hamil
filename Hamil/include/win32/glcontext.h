#pragma once

#include <win32/win32.h>

namespace win32 {

class Window;

class OGLContext {
public:
  void makeCurrent();

private:
  friend Window;

  OGLContext(void *hdc, void *hglrc);

  void * /* HDC */ m_hdc;
  void * /* HGLRC */ m_hglrc;
};

}
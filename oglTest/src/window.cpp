#include "window.h"

#include <cassert>
#include <algorithm>
#include <string>

#include <windowsx.h>

#include <GL/glew.h>
#include <GL/wglew.h>

#pragma comment(lib, "opengl32.lib")

namespace win32 {

Window::Window(int width, int height)
{
  WNDCLASSEX wndClass;
  auto ptr = (unsigned char *)&wndClass;

  HINSTANCE hInstance = GetModuleHandle(nullptr);

  std::fill(ptr, ptr+sizeof(WNDCLASSEX), 0);

  wndClass.cbSize        = sizeof(WNDCLASSEX);
  wndClass.style         = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
  wndClass.lpfnWndProc   = &Window::WindowProc;
  wndClass.cbWndExtra    = sizeof(this);
  wndClass.hInstance     = hInstance;
  wndClass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
  wndClass.lpszClassName = wnd_class_name();

  ATOM atom = RegisterClassEx(&wndClass);

  assert(atom && "failed to register window class!");

  RECT rc = { 0, 0, width, height };
  AdjustWindowRect(&rc, WS_CAPTION|WS_SYSMENU, false);

  m_hwnd = CreateWindow(wnd_class_name(), wnd_name(), WS_CAPTION|WS_SYSMENU,
                      CW_USEDEFAULT, CW_USEDEFAULT, rc.right-rc.left, rc.bottom-rc.top,
                      nullptr, nullptr, hInstance, 0);

  assert(m_hwnd && "failed to create window!");

  ShowWindow(m_hwnd, SW_SHOW);
  SetWindowLongPtr(m_hwnd, GWLP_USERDATA, (LONG_PTR)this);
}

bool Window::processMessages()
{
  MSG msg;
  while(PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE) != 0) {
    if(msg.message == WM_QUIT) return false;

    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  return true;
}

void Window::swapBuffers()
{
  wglSwapLayerBuffers(GetDC(m_hwnd), WGL_SWAP_MAIN_PLANE);
}

Input::Ptr Window::getInput()
{
  return m_input_man.getInput();
}

void Window::setMouseSpeed(float speed)
{
  m_input_man.setMouseSpeed(speed);
}

void Window::captureMouse()
{
  RECT rc;
  POINT pt = { 0, 0 };

  ClientToScreen(m_hwnd, &pt);
  GetClientRect(m_hwnd, &rc);

  SetCursorPos(pt.x, pt.y);

  rc.top += pt.y; rc.bottom += pt.y;
  rc.left += pt.x; rc.right += pt.x;
  ClipCursor(&rc);

  while(ShowCursor(FALSE) >= 0);
}

void Window::releaseMouse()
{
  ClipCursor(nullptr);

  while(ShowCursor(TRUE) < 0);
}

void Window::quit()
{
  PostQuitMessage(0);
}

HGLRC Window::ogl_create_context(HWND hWnd)
{
  PIXELFORMATDESCRIPTOR pfd = {
    sizeof(PIXELFORMATDESCRIPTOR),
    1,
    PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER|PFD_DEPTH_DONTCARE,
    PFD_TYPE_RGBA,
    16,
    0, 0, 0, 0, 0, 0, 0, 0,
    0,
    0, 0, 0, 0,
    0, 0,
    0,
    PFD_MAIN_PLANE,
    0, 0, 0, 0,
  };

  HDC hdc = GetDC(hWnd);
  int pixel_format = ChoosePixelFormat(hdc, &pfd);

  assert(pixel_format && "cannot choose pixel format!");
  
  SetPixelFormat(hdc, pixel_format, &pfd);

  HGLRC context = wglCreateContext(hdc);
  assert(context && "failed to create ogl context");

  BOOL result = wglMakeCurrent(hdc, context);
  assert(result && "failed to make context current");
  
  GLenum err = glewInit();
  if(err != GLEW_OK) return nullptr;

  wglSwapIntervalEXT(0);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  return context;
}

LRESULT Window::WindowProc(HWND hWnd, UINT uMsg, WPARAM wparam, LPARAM lparam)
{
  auto self = (Window *)GetWindowLongPtr(hWnd, GWLP_USERDATA);

  switch(uMsg) {
  case WM_CREATE:
    ogl_create_context(hWnd);
    return 0;

  case WM_PAINT: {
    PAINTSTRUCT ps;
    BeginPaint(hWnd, &ps);
    EndPaint(hWnd, &ps);
    return 0;
  }

  case WM_INPUT:
    self->m_input_man.process((void *)lparam);
    return 0;

  case WM_SETFOCUS:
  case WM_ACTIVATE:
    if(self) self->captureMouse();
    return 0;

  case WM_KILLFOCUS:
    self->releaseMouse();
    return 0;

  case WM_DESTROY:
    self->quit();
    return 0;

  default: return DefWindowProc(hWnd, uMsg, wparam, lparam);
  }

  return 0;
}

}

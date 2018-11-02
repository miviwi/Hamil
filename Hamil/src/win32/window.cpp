#include <win32/window.h>
#include <win32/panic.h>

#include <cassert>
#include <algorithm>
#include <string>

#include <windowsx.h>

#include <GL/gl3w.h>
#include <GL/wgl.h>

#pragma comment(lib, "opengl32.lib")

namespace win32 {

PFNWGLSWAPINTERVALEXTPROC SwapIntervalEXT;
PFNWGLCREATECONTEXTATTRIBSARBPROC CreateContextAttribsARB;

static HGLRC ogl_create_context(HWND hWnd);

static void APIENTRY ogl_debug_callback(GLenum source, GLenum type, GLuint id,
                                        GLenum severity, GLsizei length, GLchar *msg, const void *user);

#if !defined(NDEBUG)
constexpr int ContextFlags = WGL_CONTEXT_DEBUG_BIT_ARB;
#else
constexpr int ContextFlags = 0;
#endif

constexpr int ContextProfile = WGL_CONTEXT_CORE_PROFILE_BIT_ARB;

static const int ContextAttribs[] = {
  WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
  WGL_CONTEXT_MINOR_VERSION_ARB, 3,
  WGL_CONTEXT_PROFILE_MASK_ARB, ContextProfile,
  WGL_CONTEXT_FLAGS_ARB, ContextFlags,
  0
};

static void get_wgl_extension_addresses()
{
  SwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
  CreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");
}

Window::Window(int width, int height) :
  m_hwnd(nullptr),
  m_hglrc(nullptr),
  m_width(width), m_height(height),
  m_thread(Thread::current_thread_id())
{
  HINSTANCE hInstance = GetModuleHandle(nullptr);

  registerClass(hInstance);
  m_hwnd = createWindow(hInstance, width, height);
}

Window::~Window()
{
  if(deref()) return;

  wglMakeCurrent(nullptr, nullptr);
  wglDeleteContext(m_hglrc);

  DestroyWindow(m_hwnd);
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

void Window::swapInterval(unsigned interval)
{
  SwapIntervalEXT(interval);
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
  SetCapture(m_hwnd);
  resetMouse();

  while(ShowCursor(FALSE) >= 0);
}

void Window::releaseMouse()
{
  ReleaseCapture();

  while(ShowCursor(TRUE) < 0);
}

void Window::resetMouse()
{
  POINT pt = { 0, 0 };

  ClientToScreen(m_hwnd, &pt);
  SetCursorPos(pt.x + (m_width/2), pt.y + (m_height/2));
}

void Window::quit()
{
  PostMessage(m_hwnd, WM_CLOSE, 0, 0);
}

static ATOM p_atom = INVALID_ATOM;
ATOM Window::registerClass(HINSTANCE hInstance)
{
  // The class has already been registered
  if(p_atom != INVALID_ATOM) return p_atom;

  WNDCLASSEX wndClass;

  ZeroMemory(&wndClass, sizeof(wndClass));

  wndClass.cbSize = sizeof(WNDCLASSEX);
  wndClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
  wndClass.lpfnWndProc = &Window::WindowProc;
  wndClass.cbWndExtra = sizeof(this);
  wndClass.hInstance = hInstance;
  wndClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
  wndClass.lpszClassName = wnd_class_name();

  ATOM atom = RegisterClassEx(&wndClass);
  assert(atom && "failed to register window class!");

  // Store the class ATOM
  return p_atom = atom;
}

HWND Window::createWindow(HINSTANCE hInstance, int width, int height)
{
  RECT rc = { 0, 0, width, height };
  AdjustWindowRect(&rc, WS_CAPTION|WS_SYSMENU, false);

  HWND hwnd = CreateWindow(wnd_class_name(), wnd_name(), WS_CAPTION|WS_SYSMENU,
                      CW_USEDEFAULT, CW_USEDEFAULT, rc.right-rc.left, rc.bottom-rc.top,
                      nullptr, nullptr, hInstance, this);
  assert(hwnd && "failed to create window!");

  ShowWindow(hwnd, SW_SHOW);
  SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)this);

  return hwnd;
}

HGLRC ogl_create_context(HWND hWnd)
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

  HGLRC temp_context = wglCreateContext(hdc);
  assert(temp_context && "failed to create temporary ogl context");

  wglMakeCurrent(hdc, temp_context);
  
  int err = gl3wInit();
  if(err) panic("Failed to initialize gl3w!", GL3WInitError);

  if(!gl3wIsSupported(3, 3)) panic("OpenGL version >= 3.3 required!", OpenGL3_3NotSupportedError);

  get_wgl_extension_addresses();

  auto context = CreateContextAttribsARB(hdc, nullptr, ContextAttribs);

  wglMakeCurrent(nullptr, nullptr);
  wglDeleteContext(temp_context);

  wglMakeCurrent(hdc, context);

  SwapIntervalEXT(1);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

#if !defined(NDEBUG)
  glEnable(GL_DEBUG_OUTPUT);
  glDebugMessageCallback((GLDEBUGPROC)ogl_debug_callback, nullptr);
#endif

  glGetError(); // clear the error indicator

  return context;
}

GlContext Window::acquireGlContext()
{
  assert(Thread::current_thread_id() == m_thread &&
    "Window::acquireGlContext() called on the wrong thread!");

  HDC hdc = GetDC(m_hwnd);
  HGLRC hglrc = CreateContextAttribsARB(hdc, m_hglrc, ContextAttribs);

  return GlContext(hdc, hglrc);
}

LRESULT Window::WindowProc(HWND hWnd, UINT uMsg, WPARAM wparam, LPARAM lparam)
{
  auto self = (Window *)GetWindowLongPtr(hWnd, GWLP_USERDATA);

  switch(uMsg) {
  case WM_CREATE: {
    // 'this' is specified as the 'lpParam' to CreateWindow()
    auto create_struct = (LPCREATESTRUCT)lparam;
    auto window = (Window *)create_struct->lpCreateParams;

    window->m_hglrc = ogl_create_context(hWnd);
    return 0;
  }

  case WM_PAINT: {
    PAINTSTRUCT ps;
    BeginPaint(hWnd, &ps);
    EndPaint(hWnd, &ps);
    return 0;
  }

  case WM_INPUT:
    self->m_input_man.process((void *)lparam);
    self->resetMouse();
    return 0;

  case WM_SETFOCUS:
  case WM_ACTIVATE:
    if(self) self->captureMouse();
    return 0;

  case WM_KILLFOCUS:
    self->releaseMouse();
    return 0;

  case WM_DESTROY:
    PostQuitMessage(0);
    return 0;

  default: return DefWindowProc(hWnd, uMsg, wparam, lparam);
  }

  return 0;
}

static char dbg_buf[1024];

static void APIENTRY ogl_debug_callback(GLenum source, GLenum type, GLuint id,
                               GLenum severity, GLsizei length, GLchar *msg, const void *user)
{
  const char *source_str = "";
  switch(source) {
  case GL_DEBUG_SOURCE_API:             source_str = "GL_API"; break;
  case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   source_str = "GL_WINDOW_SYSTEM"; break;
  case GL_DEBUG_SOURCE_SHADER_COMPILER: source_str = "GL_SHADER_COMPILER"; break;
  case GL_DEBUG_SOURCE_APPLICATION:     source_str = "GL_APPLICATION"; break;
  case GL_DEBUG_SOURCE_THIRD_PARTY:     source_str = "GL_THIRD_PARTY"; break;
  case GL_DEBUG_SOURCE_OTHER:           source_str = "GL_OTHER"; break;
  }

  const char *type_str = "";
  switch(type) {
  case GL_DEBUG_TYPE_ERROR:               type_str = "error!"; break;
  case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: type_str = "deprecated"; break;
  case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  type_str = "undefined!"; break;
  case GL_DEBUG_TYPE_PORTABILITY:         type_str = "portability"; break;
  case GL_DEBUG_TYPE_PERFORMANCE:         type_str = "performance"; break;
  case GL_DEBUG_TYPE_OTHER:               type_str = "other"; break;
  }

  const char *severity_str = "";
  switch(severity) {
  case GL_DEBUG_SEVERITY_HIGH:         severity_str = "!!!"; break;
  case GL_DEBUG_SEVERITY_MEDIUM:       severity_str = "!!"; break;
  case GL_DEBUG_SEVERITY_LOW:          severity_str = "!"; break;
  case GL_DEBUG_SEVERITY_NOTIFICATION: severity_str = "?"; break;
  }

  sprintf_s(dbg_buf, "%s (%s, %s): %s\n", source_str, severity_str, type_str, msg);

  OutputDebugStringA(dbg_buf);
}

}

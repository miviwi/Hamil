#include <sysv/window.h>
#include <sysv/sysv.h>
#include <sysv/x11.h>
#include <sysv/inputman.h>
#include <os/panic.h>
#include <gx/context.h>

#include <cassert>

// X11/xcb headers
#include <xcb/xcb.h>
#include <X11/Xlib.h>
#include <X11/Xlib-xcb.h>
#include <X11/keysymdef.h>

namespace sysv {

using x11_detail::x11;

struct X11Window {
  X11Window(unsigned width, unsigned height);
  ~X11Window();

  // Returns 'false' if creating the colormap failed
  bool createColormap(xcb_visualid_t visual = 0);

  bool createWindow(unsigned width, unsigned height, u8 depth = 0, xcb_visualid_t visual = 0);

  xcb_connection_t *connection;
  xcb_screen_t *screen;

  X11Id window = X11InvalidId;
  X11Id colormap = X11InvalidId;
};

X11Window::X11Window(unsigned width, unsigned height) :
  connection(nullptr), screen(nullptr)
{
  connection = x11().connection<xcb_connection_t>();
  screen = x11().screen<xcb_screen_t>();

  auto create_colormap_success = createColormap();
  if(!create_colormap_success) os::panic("xcb_create_colormap() failed!", os::XCBError);

  auto create_window_success = createWindow(width, height);
  if(!create_window_success) os::panic("xcb_create_window() failed!", os::XCBError);
}

X11Window::~X11Window()
{
#if 0
  xcb_destroy_window(connection, window);
  xcb_free_colormap(connection, colormap);
#endif
}

bool X11Window::createColormap(xcb_visualid_t visual)
{
  colormap = x11().genId();

  auto colormap_cookie = xcb_create_colormap(
      connection, XCB_COLORMAP_ALLOC_NONE, colormap,
      screen->root, visual ? visual : screen->root_visual
  );
  auto err = xcb_request_check(connection, colormap_cookie);
  if(!err) return true;

  // There was an error - cleanup and signal failure
  free(err);
  return false;
}

bool X11Window::createWindow(unsigned width, unsigned height, u8 depth, xcb_visualid_t visual)
{
  window = x11().genId();

  u32 mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK | XCB_CW_COLORMAP;
  u32 args[] = {
    0xFF0000,                                                     /* XCB_CW_BACK_PIXEL */
    XCB_EVENT_MASK_EXPOSURE                                       /* XCB_CW_EVENT_MASK */
    | XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE
    | XCB_EVENT_MASK_POINTER_MOTION | XCB_EVENT_MASK_BUTTON_MOTION
    | XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE,
    colormap,                                                     /* XCB_CW_COLORMAP */
  };

  auto window_cookie = xcb_create_window_checked(
      connection, visual ? depth : screen->root_depth, window, screen->root, 
      0 /* x */, 0 /* y */, width, height, 0,
      XCB_WINDOW_CLASS_INPUT_OUTPUT, visual ? visual : screen->root_visual, mask, args
  );
  auto err = xcb_request_check(connection, window_cookie);
  if(!err) return true;

  // There was an error - cleanup and signal failure
  free(err);
  return false;
}

Window::Window(int width, int height) :
  os::Window(width, height),
  p(nullptr)
{
  assert((width > 0) && (height > 0) &&
      "attempted to create a Window width negative or 0 width/height");
  p = new X11Window((unsigned)width, (unsigned)height);

  xcb_map_window(x11().connection<xcb_connection_t>(), p->window);

  x11().flush();
}

Window::~Window()
{
  delete p;
}

void *Window::nativeHandle() const
{
  return nullptr;
}

void Window::swapBuffers()
{
}

void Window::swapInterval(unsigned interval)
{
}

void Window::captureMouse()
{
}

void Window::releaseMouse()
{
}

void Window::resetMouse()
{
}

void Window::quit()
{
}

void Window::destroy()
{
}

bool Window::doProcessMessages()
{
#if __sysv
  // TODO: implement :)

  return false;
#else
  return false;
#endif
}

os::InputDeviceManager *Window::acquireInputManager()
{
  return new sysv::InputDeviceManager();
}

}

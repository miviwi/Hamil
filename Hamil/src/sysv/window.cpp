#include <sysv/window.h>
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

enum {
  X11InvalidId = ~0u,
};

struct X11Connection {
  X11Connection();
  X11Connection(const X11Connection&) = delete;
  ~X11Connection();

  Display *display = nullptr;
  int default_screen = -1;

  xcb_connection_t *connection = nullptr;
  const xcb_setup_t *setup = nullptr;
  xcb_screen_t *screen = nullptr;

  u32 genId();
};

X11Connection::X11Connection()
{
  display = XOpenDisplay(nullptr);
  if(!display) os::panic("failed to open an X11 Display!", os::XOpenDisplayError);

  default_screen = DefaultScreen(display);

  connection = XGetXCBConnection(display);
  if(!connection) os::panic("failed to get an X11 xcb_connection_t!" , os::GetXCBConnectionError);

  XSetEventQueueOwner(display, XCBOwnsEventQueue);

  setup = xcb_get_setup(connection);
  if(!setup) os::panic("xcb_get_setup() failed!", os::XCBError);

  auto roots_it = xcb_setup_roots_iterator(setup);
  while(roots_it.index < default_screen) {
    if(!roots_it.rem) os::panic("couldn't find root window of default screen!", os::XCBError);

    xcb_screen_next(&roots_it);
  }

  screen = roots_it.data;
}

X11Connection::~X11Connection()
{
  xcb_disconnect(connection);
}

u32 X11Connection::genId()
{
  return xcb_generate_id(connection);
}

X11Connection *Window::p_x11 = nullptr;

struct X11Window {
  X11Window(X11Connection& x11, unsigned width, unsigned height);
  ~X11Window();

  // Returns 'false' if creating the colormap failed
  bool createColormap(xcb_visualid_t visual = 0);

  bool createWindow(unsigned width, unsigned height, u8 depth = 0, xcb_visualid_t visual = 0);

  X11Connection& x11;

  u32 window = X11InvalidId;
  u32 colormap = X11InvalidId;
};

X11Window::X11Window(X11Connection& x11_, unsigned width, unsigned height) :
  x11(x11_)
{
  auto create_colormap_success = createColormap();
  if(!create_colormap_success) os::panic("xcb_create_colormap() failed!", os::XCBError);

  auto create_window_success = createWindow(width, height);
  if(!create_window_success) os::panic("xcb_create_window() failed!", os::XCBError);
}

X11Window::~X11Window()
{
  xcb_destroy_window(x11.connection, window);
  xcb_free_colormap(x11.connection, colormap);
}

bool X11Window::createColormap(xcb_visualid_t visual)
{
  colormap = x11.genId();

  auto colormap_cookie = xcb_create_colormap(
      x11.connection, XCB_COLORMAP_ALLOC_NONE, colormap,
      x11.screen->root, visual ? visual : x11.screen->root_visual
  );
  auto err = xcb_request_check(x11.connection, colormap_cookie);
  if(!err) return true;

  // There was an error - cleanup and signal failure
  free(err);
  return false;
}

bool X11Window::createWindow(unsigned width, unsigned height, u8 depth, xcb_visualid_t visual)
{
  window = x11.genId();

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
      x11.connection, visual ? depth : x11.screen->root_depth, window, x11.screen->root, 
      0 /* x */, 0 /* y */, width, height, 0,
      XCB_WINDOW_CLASS_INPUT_OUTPUT, visual ? visual : x11.screen->root_visual, mask, args
  );
  auto err = xcb_request_check(x11.connection, window_cookie);
  if(!err) return true;

  // There was an error - cleanup and signal failure
  free(err);
  return false;
}

Window::Window(int width, int height) :
  os::Window(width, height),
  p(nullptr)
{
  if(!p_x11) p_x11 = new X11Connection();

  assert((width > 0) && (height > 0) &&
      "attempted to create a Window width negative or 0 width/height");
  p = new X11Window(*p_x11, (unsigned)width, (unsigned)height);

  xcb_map_window(p_x11->connection, p->window);
  xcb_flush(p_x11->connection);
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

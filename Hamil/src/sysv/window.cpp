#include <sysv/window.h>
#include <sysv/sysv.h>
#include <sysv/x11.h>
#include <sysv/inputman.h>
#include <sysv/glcontext.h>
#include <os/panic.h>

#include <config>

#include <array>

#include <cassert>
#include <cstdlib>
#include <cstring>

// X11/xcb headers
#if __sysv
#  include <xcb/xcb.h>
#  include <xcb/xproto.h>
#  include <xcb/xcb_event.h>
#  include <X11/Xlib.h>
#  include <X11/Xlib-xcb.h>
#  include <X11/keysymdef.h>
#endif

namespace sysv {

using x11_detail::x11;

static const char X11_WM_PROTOCOLS[]     = "WM_PROTOCOLS";
static const char X11_WM_DELETE_WINDOW[] = "WM_DELETE_WINDOW";
//static const char X11_WM_TAKE_FOCUS[]    = "WM_TAKE_FOCUS";

struct X11Window {
  X11Window(unsigned width, unsigned height);
  ~X11Window();

  // Returns 'false' if creating the colormap failed
  bool createColormap(xcb_visualid_t visual = 0);

  bool createWindow(unsigned width, unsigned height, u8 depth = 0, xcb_visualid_t visual = 0);

  void show();

  void destroy();

  xcb_connection_t *connection = nullptr;
  xcb_screen_t *screen = nullptr;

  xcb_atom_t atom_delete_window = X11InvalidId;

  X11Id window = X11InvalidId;
  X11Id colormap = X11InvalidId;
};

X11Window::X11Window(unsigned width, unsigned height)
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
  destroy();
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
    XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_STRUCTURE_NOTIFY     /* XCB_CW_EVENT_MASK */
    | XCB_EVENT_MASK_FOCUS_CHANGE,
    /*
    | XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE
    | XCB_EVENT_MASK_POINTER_MOTION | XCB_EVENT_MASK_BUTTON_MOTION
    | XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE,
    */
    colormap,                                                     /* XCB_CW_COLORMAP */
  };

  auto window_cookie = xcb_create_window_checked(
      connection, visual ? depth : screen->root_depth, window, screen->root, 
      0 /* x */, 0 /* y */, width, height, 0,
      XCB_WINDOW_CLASS_INPUT_OUTPUT, visual ? visual : screen->root_visual, mask, args
  );
  auto create_window_err = xcb_request_check(connection, window_cookie);
  if(create_window_err) {  // There was an error - cleanup and signal failure
    free(create_window_err);

    return false;
  }

  auto protocols_cookie = xcb_intern_atom(
      connection, 1 /* only_if_exists */,
      sizeof(X11_WM_PROTOCOLS)-1 /* don't include '\0' */, X11_WM_PROTOCOLS
  );
  auto delete_window_cookie = xcb_intern_atom(
      connection, 0 /* only_if_exists */,
      sizeof(X11_WM_DELETE_WINDOW)-1 /* don't include '\0' */, X11_WM_DELETE_WINDOW
  );

  xcb_generic_error_t *intern_protocols_err = nullptr;
  xcb_generic_error_t *intern_delete_window_err = nullptr;

  auto protocols_reply = xcb_intern_atom_reply(
      connection, protocols_cookie, &intern_protocols_err
  );
  auto delete_reply = xcb_intern_atom_reply(
      connection, delete_window_cookie, &intern_delete_window_err
  );

  if(intern_protocols_err || intern_delete_window_err) {
    free(intern_protocols_err);
    free(intern_delete_window_err);

    return false;
  }

  atom_delete_window = delete_reply->atom;

  const std::array<xcb_atom_t, 1> wm_protocols = {
    atom_delete_window,
  };

  auto change_protocols_cookie = xcb_change_property_checked(
      connection, XCB_PROP_MODE_REPLACE, window,
      protocols_reply->atom, XCB_ATOM_ATOM,
      32 /* bits per list item */, wm_protocols.size() /* num list items */, wm_protocols.data()
  );

  // Don't need the replies anymore
  free(protocols_reply);
  free(delete_reply);

  auto change_protocols_err = xcb_request_check(connection, change_protocols_cookie);
  if(change_protocols_err) {
    free(change_protocols_err);

    return false;
  }

  return true;   // No errors
}

void X11Window::show()
{
  xcb_map_window(connection, window);
}

void X11Window::destroy()
{
  xcb_destroy_window(connection, window);
  xcb_free_colormap(connection, colormap);

  window = colormap = X11InvalidId;
}

Window::Window(int width, int height) :
  os::Window(width, height),
  m_dimensions({ width, height }),
  p(nullptr)
{
  assert((width > 0) && (height > 0) &&
      "attempted to create a Window width negative or 0 width/height");
  p = new X11Window((unsigned)width, (unsigned)height);

  p->show();

  x11().flush();
}

Window::~Window()
{
}

void *Window::nativeHandle() const
{
  return (void *)(uintptr_t)p->window;
}

void Window::swapBuffers()
{
  auto& gl_context = (sysv::GLContext&)gx::GLContext::current();

  gl_context.swapBuffers();
}

void Window::swapInterval(unsigned interval)
{
  auto& gl_context = (sysv::GLContext&)gx::GLContext::current();

  gl_context.swapInterval(interval);
}

void Window::captureMouse()
{
  xcb_grab_pointer(
      p->connection, 0, /* owner_events */  // The mouse,keyboard I/O is handled via libevdev anyway
      p->window, 0, /* event_mask */
      XCB_GRAB_MODE_SYNC, /* pointer_mode */ XCB_GRAB_MODE_SYNC, /* keyboard_mode */
      p->window, /* confine_to */ XCB_NONE, /* cursor */
      XCB_CURRENT_TIME
  );

  xcb_flush(p->connection);
}

void Window::releaseMouse()
{
}

void Window::resetMouse()
{
  // TODO: store the geometry on XCB_CONFIGURE_NOTIFY events
  auto get_geometry_cookie = xcb_get_geometry(p->connection, p->window);

  xcb_generic_error_t *get_geometry_err = nullptr;
  xcb_get_geometry_reply_t *get_geometry_reply = xcb_get_geometry_reply(
      p->connection, get_geometry_cookie, &get_geometry_err
  );
  assert(!get_geometry_err);

  xcb_warp_pointer(
      p->connection,
      XCB_NONE, /* src_window */ p->window, /* dst_window */
      0, /* src_x */ 0, /* src_y */ 0, /* src_width */ 0, /* src_height */
      get_geometry_reply->width/2, get_geometry_reply->height/2
  );

  free(get_geometry_reply);
}

void Window::quit()
{
  auto event = (xcb_client_message_event_t *)malloc(sizeof(xcb_client_message_event_t));
  memset(event, 0, sizeof(*event));

  event->response_type = XCB_CLIENT_MESSAGE;

  event->type = XCB_ATOM_ATOM;
  event->format = 32;
  event->data.data32[0] = p->atom_delete_window;

  auto send_event_cookie = xcb_send_event_checked(
      p->connection, 1 /* propagate */, p->window,
      /* event_mask */ 0,
      (char *)event);
  xcb_flush(p->connection);

  auto send_event_err = xcb_request_check(p->connection, send_event_cookie);
  assert(!send_event_err);

  free(event);
}

void Window::destroy()
{
  delete p;

  p = nullptr;
  m_dimensions = { -1, -1 };
}

bool Window::doProcessMessages()
{
#if __sysv
  auto connection = x11().connection<xcb_connection_t>();

  resetMouse();

  while(xcb_generic_event_t *ev = xcb_poll_for_event(connection)) {
    switch(XCB_EVENT_RESPONSE_TYPE(ev)) {
    case XCB_CLIENT_MESSAGE: {
      const auto& client_message = *(xcb_client_message_event_t *)ev;

      auto data = client_message.data.data32[0];
      if(data == p->atom_delete_window) {
        free(ev);

        return false;
      }

      break;
    }

    case XCB_FOCUS_IN:
      captureMouse();
      break;

    case XCB_FOCUS_OUT:
      releaseMouse();
      break; 
    }

    free(ev);
  }

  return true;
#else
  return false;
#endif
}

os::InputDeviceManager *Window::acquireInputManager()
{
  return new sysv::InputDeviceManager();
}

bool Window::recreateWithVisual(u8 depth, u32 visual)
{
  p->destroy();

  // When creating a window with an arbitrarily chosen visual
  //   we must also create a colormap with a visual which
  //   is the same as the one chosen for the window.
  //  - Failing to do so results in a BadMatch X server
  //    error when calling xcb_create_window()/XCreateWindow()
  if(!p->createColormap(visual)) return false;
  if(!p->createWindow(m_dimensions.x, m_dimensions.y, depth, visual)) return false;

  // Newly created windows are
  //   invisible by default
  p->show();

  x11().flush();     // Make sure the requests reach the X server

  return true;
}

}

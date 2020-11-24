#include <sysv/x11.h>
#include <os/panic.h>

#include <config>

#include <cassert>

// X11/xcb headers
#if __sysv
#  include <xcb/xcb.h>
#  include <X11/Xlib.h>
#  include <X11/Xlib-xcb.h>
#  include <X11/keysymdef.h>
#endif

namespace sysv {

struct X11ConnectionData {
  Display *display = nullptr;
  int default_screen = -1;

  xcb_connection_t *connection = nullptr;
  const xcb_setup_t *setup = nullptr;
  xcb_screen_t *screen = nullptr;
};

X11Connection *X11Connection::connect()
{
  return new X11Connection();
}

X11Connection::X11Connection() :
  m_data(nullptr)
{
  m_data = new X11ConnectionData();

  auto display = XOpenDisplay(nullptr);
  if(!display) os::panic("failed to open an X11 Display!", os::XOpenDisplayError);

  // Create XCB connection (xcb_connection_t *) from an Xlib Display* handle
  auto connection = XGetXCBConnection(display);
  if(!connection) os::panic("failed to get an X11 xcb_connection_t!" , os::GetXCBConnectionError);

  XSetEventQueueOwner(display, XCBOwnsEventQueue);

  auto setup = xcb_get_setup(connection);
  if(!setup) os::panic("xcb_get_setup() failed!", os::XCBError);

  auto roots_it = xcb_setup_roots_iterator(setup);
  auto default_screen = DefaultScreen(display);
  while(roots_it.index < default_screen) {
    if(!roots_it.rem) os::panic("couldn't find root window of default screen!", os::XCBError);

    xcb_screen_next(&roots_it);
  }

  // Store Xlib-specific data
  data().display = display;
  data().default_screen = default_screen;

  // Store XCB-specific data
  data().connection = connection;
  data().setup = setup;
  data().screen = roots_it.data;
}

X11Connection::~X11Connection()
{
  disconnect();
}

X11Id X11Connection::genId()
{
  assert(m_data);

  return xcb_generate_id(data().connection);
}

X11Connection& X11Connection::flush()
{
  assert(m_data &&  "X11Connection::flush() called before connect()!");

  xcb_flush(data().connection);

  return *this;
}

int X11Connection::defaultScreen()
{
  assert(m_data);

  return data().default_screen;
}

void X11Connection::disconnect()
{
  if(!m_data) return;

  xcb_disconnect(data().connection);

  delete m_data;
  m_data = nullptr;
}

X11ConnectionData& X11Connection::data()
{
  assert(m_data);

  return *m_data;
}

const X11ConnectionData& X11Connection::data() const
{
  assert(m_data);

  return *m_data;
}

void *X11Connection::connection()
{
  assert(m_data);

  return data().connection;
}

void *X11Connection::screen()
{
  assert(m_data);

  return data().screen;
}

void *X11Connection::xlibDisplay()
{
  assert(m_data);

  return data().display;
}

}

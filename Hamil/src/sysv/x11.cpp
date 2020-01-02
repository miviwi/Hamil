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
#if __sysv
  Display *display = nullptr;
  int default_screen = -1;

  xcb_connection_t *connection = nullptr;
  const xcb_setup_t *setup = nullptr;
  xcb_screen_t *screen = nullptr;
#endif
};

X11Connection *X11Connection::connect()
{
  return new X11Connection();
}

X11Connection::X11Connection() :
  m_data(nullptr)
{
  m_data = new X11ConnectionData();

#if __sysv
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
#endif
}

X11Connection::~X11Connection()
{
#if __sysv
  xcb_disconnect(data().connection);
#endif

  delete m_data;
}

X11Id X11Connection::genId()
{
#if __sysv
  assert(m_data);

  return xcb_generate_id(data().connection);
#else
  return X11InvalidId;
#endif
}

X11Connection& X11Connection::flush()
{
#if __sysv
  assert(m_data);

  xcb_flush(data().connection);
#endif
  return *this;
}

int X11Connection::defaultScreen()
{
  assert(m_data);

#if __sysv
  return data().default_screen;
#else
  return -1;
#endif
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

#if __sysv
  return data().connection;
#else
  return nullptr;
#endif
}

void *X11Connection::screen()
{
  assert(m_data);

#if __sysv
  return data().screen;
#else
  return nullptr;
#endif
}

void *X11Connection::xlibDisplay()
{
  assert(m_data);

#if __sysv
  return data().display;
#else
  return nullptr;
#endif
}

}

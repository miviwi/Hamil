#pragma once

#include <sysv/sysv.h>

namespace sysv {

using X11Id = u32;

enum : X11Id {
  X11InvalidId = ~0u,
};

// PIMPL struct
struct X11ConnectionData;

// An instance is created during sysv::init(), after which it is accessible
//   via a call to sysv::x11_detail::x11()
//  - Cleaned up automatically by sysv::finalize()
class X11Connection {
public:
  X11Connection(const X11Connection&) = delete;
  ~X11Connection();

  X11Id genId();

  X11Connection& flush();

  // Returns an xcb_connection_t*
  //   - T must ALWAYS be xcb_connection_t
  template <typename T /* = xcb_connection_t */>
  T *connection()
  {
    return (T *)connection();
  }
  // Returns an xcb_screen_t*
  //   - T must ALWAYS be xcb_screen_t
  template <typename T /* = xcb_screen_t */>
  T *screen()
  {
    return (T *)screen();
  }

  template <typename T /* Display */>
  T *xlibDisplay()
  {
    return (T *)xlibDisplay();
  }

  int defaultScreen();

  void disconnect();

private:
  friend void init();

  static X11Connection *connect();

  X11Connection();

  X11ConnectionData& data();
  const X11ConnectionData& data() const;

  // Use ONLY via call to connection<xcb_connection_t>()
  void *connection();
  // Use ONLY via call to screen<xcb_screen_t>()
  void *screen();

  // Use ONLY vis call to xlibDisplay<Display>()
  void *xlibDisplay();

  X11ConnectionData *m_data;
};

}

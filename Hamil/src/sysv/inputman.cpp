#include <sysv/inputman.h>
#include <sysv/time.h>
#include <os/input.h>
#include <os/panic.h>
#include <util/format.h>

#include <config>

#include <cassert>
#include <cerrno>
#include <cstring>

#include <optional>

// POSIX headers
#if __sysv
#  include <unistd.h>
#  include <fcntl.h>
#  include <glob.h>
#  include <sys/stat.h>
#endif

// X11/xcb headers
#if __sysv 
#  include <xcb/xcb.h>
#  include <X11/Xlib.h>
#  include <X11/Xlib-xcb.h>
#  include <X11/keysymdef.h>
#endif

// libevdev
#if __sysv
# include <libevdev/libevdev.h>
#endif

namespace sysv {

struct LibevdevDevice {
#if __sysv
  int fd = -1;
  struct libevdev *device = nullptr;
#endif
};

struct LibevdevEvent {
#if __sysv
  struct input_event ev;
#endif
};

enum LibevdevKeyboardEventValue {
  KeyboardEventKeyDown = 1,
  KeyboardEventKeyUp   = 0,
};

struct InputDeviceManagerData {
#if __sysv
  xcb_connection_t *connection = nullptr;

  LibevdevDevice mouse;
  LibevdevDevice kb;
#endif
};

static LibevdevDevice open_device(const char *device_type)
{
  LibevdevDevice device;

  auto glob_pattern = util::fmt("/dev/input/by-id/*-event-%s", device_type);

  glob_t device_glob;
  int glob_err = glob(glob_pattern.data(), 0, nullptr, &device_glob);
  if (glob_err || device_glob.gl_pathc < 1) {
    os::panic(
        util::fmt("failed to find a %s device!", device_type).data(),
        os::LibevdevDeviceOpenError
    );
  }

  const char *device_path = device_glob.gl_pathv[0];

  int fd_device = open(device_path, O_RDONLY | O_NONBLOCK);
  if(fd_device < 0) {
    os::panic(
        util::fmt("failed to open '%s'!", device_path).data(),
        os::LibevdevDeviceOpenError
    );
  }

  struct libevdev *pdevice = nullptr;
  int device_init_err = libevdev_new_from_fd(fd_device, &pdevice);
  if(device_init_err) {
    os::panic(
        util::fmt("libevdev_new_from_fd(\"%s\") failed!", device_path).data(),
        os::LibevdevDeviceOpenError
    );
  }

  device.fd = fd_device;
  device.device = pdevice;

  globfree(&device_glob);

  return device;
}

static void close_device(const LibevdevDevice& device)
{
  libevdev_free(device.device);
  close(device.fd);
}

static std::optional<LibevdevEvent> next_event(const LibevdevDevice& libevdev_device)
{
  LibevdevEvent libevdev_event;

  auto device = libevdev_device.device;

  int next_event_err = libevdev_next_event(
      device, LIBEVDEV_READ_FLAG_NORMAL,
      &libevdev_event.ev
  );
  if(!next_event_err) {
    /*
    const char *device_name = libevdev_get_name(device);
    const auto& ev = libevdev_event.ev;

    int value = -1;
    libevdev_fetch_event_value(device, ev.type, ev.code, &value);

    printf("'%s' %s event (%s=%d)\n",
        device_name,
        libevdev_event_type_get_name(ev.type),
        libevdev_event_code_get_name(ev.type, ev.code), value
    );
    */
  } else if(next_event_err && next_event_err != -EAGAIN) {
    printf("libevdev_next_event() error %s!\n", strerror(-next_event_err));

    return std::nullopt;
  } else if(next_event_err == -EAGAIN) {
    return std::nullopt;   // No more events to process currently
  }

  return libevdev_event;
}

static int event_value(
    const LibevdevDevice& libevdev_device,
    const LibevdevEvent& libevdev_ev)
{
  const auto& ev = libevdev_ev.ev;

  int value = -1;
  int fetch_value_success = libevdev_fetch_event_value(
      libevdev_device.device,
      ev.type, ev.code,
      &value
  );
  assert(fetch_value_success);

  return value;
}

static unsigned translate_key(int code, unsigned modifiers);
static unsigned translate_sym(int code, unsigned modifiers);

InputDeviceManager::InputDeviceManager() :
  m_data(nullptr)
{
#if __sysv
  m_data = new InputDeviceManagerData();

  m_data->mouse = open_device("mouse");
  m_data->kb = open_device("kbd");

  m_kb_modifiers = 0;
  m_capslock = 0;    // FIXME: query CapsLock's current state here!
#endif
}

InputDeviceManager::~InputDeviceManager()
{
#if __sysv
  close_device(m_data->mouse);
  close_device(m_data->kb);
#endif

  delete m_data;
}

Input::Ptr InputDeviceManager::doPollInput()
{
  Input::Ptr input(nullptr, &Input::deleter);

#if __sysv
  while(std::optional<LibevdevEvent> libevdev_kb_ev = next_event(m_data->kb)) {
    const auto& ev = libevdev_kb_ev->ev;
    
    if(ev.type == EV_SYN) continue;   // Ignore Synchronization events at this stage
    if(ev.type == EV_MSC) continue;   //   ...and Miscelaneous events as well

    input = Input::Ptr(new Keyboard(), &Input::deleter);
    input->timestamp = sysv::Timers::ticks();

    auto kbi = (Keyboard *)input.get();

    int ev_value = event_value(m_data->kb, libevdev_kb_ev.value());

    u16 event = 0;
    switch(ev_value) {
    case KeyboardEventKeyUp:   event = Keyboard::KeyUp; break;
    case KeyboardEventKeyDown: event = Keyboard::KeyDown; break;
    }

    unsigned modifiers = 0;
    switch(ev.code) {
    case KEY_LEFTCTRL:
    case KEY_RIGHTCTRL: modifiers = Keyboard::Ctrl; break;

    case KEY_LEFTSHIFT:
    case KEY_RIGHTSHIFT: modifiers = Keyboard::Shift; break;

    case KEY_LEFTALT:
    case KEY_RIGHTALT: modifiers = Keyboard::Ctrl; break;

    case KEY_LEFTMETA:
    case KEY_RIGHTMETA: modifiers = Keyboard::Super; break;
    }

    if(event == Keyboard::KeyUp) {
      m_kb_modifiers &= ~modifiers;
    } else if(event == Keyboard::KeyDown) {
      m_kb_modifiers |= modifiers;

      if(ev.code == KEY_CAPSLOCK) m_capslock ^= Keyboard::CapsLock; // Toggles between Capslock and 0
    }

    kbi->event = event;
    kbi->modifiers = m_kb_modifiers | m_capslock;

    kbi->key = translate_key(ev.code, kbi->modifiers);
    kbi->sym = translate_sym(ev.code, kbi->modifiers);

    return input;    // Got an EV_KEY - so return from the loop
  }

#endif

  return input;
}

static unsigned translate_key(int code, unsigned modifiers)
{
#if __sysv
#endif

  return code;
}

static unsigned translate_sym(int code, unsigned modifiers)
{
#if __sysv
#endif

  return code;
}

}

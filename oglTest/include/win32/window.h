#pragma once

#include <win32/input.h>
#include <math/geometry.h>

#include <utility>
#include <functional>
#include <map>

#include <Windows.h>

namespace win32 {

class Window {
public:
  Window(int width, int height);
  ~Window();

  static LPWSTR wnd_class_name() { return L"Appliaction"; }
  static LPWSTR wnd_name() { return L"Application"; }

  HWND hwnd() const { return m_hwnd; }

  bool processMessages();

  void swapBuffers();
  void swapInterval(unsigned interval);

  Input::Ptr getInput();
  void setMouseSpeed(float speed);

  void captureMouse();
  void releaseMouse();
  void resetMouse();

  void quit();

private:
  static HGLRC ogl_create_context(HWND hWnd);
  static LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wparam, LPARAM lparam);

  ATOM register_class(HINSTANCE hInstance);
  HWND create_window(HINSTANCE hInstance, int width, int height);

  HWND m_hwnd;
  int m_width, m_height;

  InputDeviceManager m_input_man;
};

}
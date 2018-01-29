#pragma once

#include <win32/input.h>
#include <vmath.h>

#include <utility>
#include <functional>
#include <map>

#include <Windows.h>

namespace win32 {

class Window {
public:
  Window(int width, int height);

  static LPWSTR wnd_class_name() { return L"Appliaction"; }
  static LPWSTR wnd_name() { return L"Application"; }

  HWND hwnd() const { return m_hwnd; }

  bool processMessages();

  void swapBuffers();

  Input::Ptr getInput();
  void setMouseSpeed(float speed);

  void captureMouse();
  void releaseMouse();
  void resetMouse();

  void quit();

private:
  static HGLRC ogl_create_context(HWND hWnd);

  static LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wparam, LPARAM lparam);

  HWND m_hwnd;
  int m_width, m_height;

  InputDeviceManager m_input_man;
};

}
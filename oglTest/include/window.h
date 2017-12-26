#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "input.h"
#include "vmath.h"

#include <utility>
#include <functional>
#include <map>

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
  ivec2 getMousePos();

  void captureMouse();
  void releaseMouse();

  void quit();

private:
  static HGLRC ogl_create_context(HWND hWnd);

  static LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wparam, LPARAM lparam);

  HWND m_hwnd;

  InputDeviceManager m_input_man;
};

}
#pragma once

namespace win32 {

enum : int {
  UnknownError = 1<<31,         // all panic codes are < 0

  // win32 related errors
  NoSSESupportError,
  QueryPerformanceCounterError,
  InputDeviceRegistartionError,

  FileOpenError,

  // OpenGL related errors
  GL3WInitError,
  OpenGL3_3NotSupportedError,
  ShaderCompileError, ShaderLinkingError,
  FramebufferError,
};

void panic(const char *reason, int exit_code);

}
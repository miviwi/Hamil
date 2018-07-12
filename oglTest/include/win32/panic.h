#pragma once

namespace win32 {

enum : int {
  UnknownError = INT_MIN,

  NoSSESupportError,
  QueryPerformanceCounterError,
  InputDeviceRegistartionError,

  FileOpenError,

  GL3WInitError,
  OpenGL3_3NotSupportedError,
  ShaderCompileError, ShaderLinkingError,
};

void panic(const char *reason, int exit_code);

}
#pragma once

namespace os {

enum : int {
  UnknownError = 1<<31,         // all panic codes are < 0

  // win32 related errors
  NoSSESupportError, NoAVXSupportError,
  QueryPerformanceCounterError,
  InputDeviceRegistartionError,

  // sysv related error
  XOpenDisplayError, GetXCBConnectionError, XCBError,
  GLXError,

  FileOpenError,

  // OpenGL related errors
  GL3WInitError,
  OpenGL3_3NotSupportedError,
  ShaderCompileError, ShaderLinkingError,
  FramebufferError,

  // FreeType related errors
  FreeTypeFaceCreationError,
};

void panic(const char *reason, int exit_code);

}

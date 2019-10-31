#include <win32/clipboard.h>
#include <win32/panic.h>

#if defined(_MSVC_VER)
#  include <Windows.h>
#endif

namespace win32 {

Clipboard::Clipboard()
{
#if defined(_MSVC_VER)
    if(!OpenClipboard(nullptr)) throw OpenError();
#endif
}

Clipboard::~Clipboard()
{
#if defined(_MSVC_VER)
    CloseClipboard();
#endif
}

std::string Clipboard::string()
{
#if defined(_MSVC_VER)
  auto data = (const char *)GetClipboardData(CF_TEXT);
  if(!data) throw GetSetError();

  return data;
#else
  return "";
#endif
}

void Clipboard::string(const void *ptr, size_t sz)
{
#if defined(_MSVC_VER)
  auto hmem = GlobalAlloc(GHND, sz + 1);
  auto mem = GlobalLock(hmem);

  memcpy(mem, ptr, sz);

  GlobalUnlock(hmem);

  EmptyClipboard();
  auto data = SetClipboardData(CF_TEXT, hmem);
  if(!data) throw GetSetError();
#endif
}

void Clipboard::string(const std::string& str)
{
  string(str.data(), str.size());
}

}

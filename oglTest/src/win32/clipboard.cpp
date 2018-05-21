#include <win32/clipboard.h>
#include <win32/panic.h>

#include <Windows.h>

namespace win32 {

Clipboard::Clipboard()
{
  if(!OpenClipboard(nullptr)) throw OpenError();
}

Clipboard::~Clipboard()
{
  CloseClipboard();
}

std::string Clipboard::string()
{
  auto data = (const char *)GetClipboardData(CF_TEXT);
  if(!data) throw GetSetError();

  return data;
}

void Clipboard::string(const void *ptr, size_t sz)
{
  auto hmem = GlobalAlloc(GHND, sz + 1);
  auto mem = GlobalLock(hmem);

  memcpy(mem, ptr, sz);

  GlobalUnlock(hmem);

  EmptyClipboard();
  auto data = SetClipboardData(CF_TEXT, hmem);
  if(!data) throw GetSetError();

}

void Clipboard::string(const std::string& str)
{
  string(str.c_str(), str.size());
}

}
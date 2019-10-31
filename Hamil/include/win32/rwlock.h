#pragma once

#include <common.h>

#if !defined(__linux__)
#  include <Windows.h>
#endif

namespace win32 {

class ReaderWriterLock {
public:
  ReaderWriterLock();
  ~ReaderWriterLock();

  ReaderWriterLock& acquireExclusive();
  ReaderWriterLock& acquireShared();

  bool tryAcquireExclusive();
  bool tryAcquireShared();

  ReaderWriterLock& releaseExclusive();
  ReaderWriterLock& releaseShared();

private:
#if !defined(__linux__)
  SRWLOCK m;
#endif
};

}

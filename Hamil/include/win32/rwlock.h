#pragma once

#include <common.h>

#include <Windows.h>

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
  SRWLOCK m;
};

}
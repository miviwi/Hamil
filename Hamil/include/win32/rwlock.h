#pragma once

#include <os/rwlock.h>

#include <config>

#if __win32
#  include <Windows.h>
#endif

namespace win32 {

class ReaderWriterLock final : public os::ReaderWriterLock {
public:
  ReaderWriterLock();

  virtual os::ReaderWriterLock& acquireExclusive() final;
  virtual os::ReaderWriterLock& acquireShared() final;

  virtual bool tryAcquireExclusive() final;
  virtual bool tryAcquireShared() final;

  virtual os::ReaderWriterLock& releaseExclusive() final;
  virtual os::ReaderWriterLock& releaseShared() final;

private:
#if __win32
  SRWLOCK m;
#endif
};

}

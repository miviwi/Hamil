#pragma once

#include <os/rwlock.h>

#include <config>

#if __sysv
#  include <pthread.h>
#endif

namespace sysv {

class ReaderWriterLock final : public os::ReaderWriterLock {
public:
  ReaderWriterLock();
  virtual ~ReaderWriterLock();

  virtual os::ReaderWriterLock& acquireExclusive() final;
  virtual os::ReaderWriterLock& acquireShared() final;

  virtual bool tryAcquireExclusive() final;
  virtual bool tryAcquireShared() final;

  virtual os::ReaderWriterLock& releaseExclusive() final;
  virtual os::ReaderWriterLock& releaseShared() final;

private:
#if __sysv
  pthread_rwlock_t m;
#endif
};

}

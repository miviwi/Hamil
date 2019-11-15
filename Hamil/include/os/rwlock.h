#pragma once

#include <common.h>

#include <memory>

namespace os {

class ReaderWriterLock {
public:
  using Ptr = std::unique_ptr<ReaderWriterLock>;

  static Ptr alloc();

  ReaderWriterLock() = default;
  ReaderWriterLock(const ReaderWriterLock&) = delete;
  virtual ~ReaderWriterLock() = default;

  virtual ReaderWriterLock& acquireExclusive() = 0;
  virtual ReaderWriterLock& acquireShared() = 0;

  virtual bool tryAcquireExclusive() = 0;
  virtual bool tryAcquireShared() = 0;

  virtual ReaderWriterLock& releaseExclusive() = 0;
  virtual ReaderWriterLock& releaseShared() = 0;
};

}

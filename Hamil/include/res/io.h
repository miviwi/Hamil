#pragma once

#include <common.h>

#include <util/unit.h>
#include <os/file.h>
#include <sched/job.h>
#include <sched/pool.h>

#include <string>
#include <memory>
#include <functional>
#include <utility>
#include <optional>
#include <variant>

namespace res {

class ResourceManager;
class IORequest;

// Holds file contents read from disk
//   - Can be backed by a os::FileView or just a regular buffer
class IOBuffer {
public:
  IOBuffer() = default;
  IOBuffer(const IOBuffer& other);

  IOBuffer& operator=(const IOBuffer& other);
  IOBuffer& operator=(IOBuffer&& other);

  static IOBuffer make_memory_buffer(size_t sz);

  void *get();
  template <typename T> T *get() { return (T *)get(); }

  // Returns the size of the underlying data
  size_t size() const;

  // Returns 'true' when the IOBuffer holds valid data
  operator bool() const;

  // Frees the memory which holds the underlying data
  void release();

private:
  friend ResourceManager;

  enum Type {
    Unknown,
    FileView, MemoryBuffer,
  };

  using MemoryBufferPtr = std::shared_ptr<byte[]>;

  IOBuffer(void *ptr, size_t sz);
  IOBuffer(os::FileView::Ptr view);

  void *m_ptr = nullptr;
  size_t m_sz = 0;

  std::variant<std::monostate,
    os::FileView::Ptr, MemoryBufferPtr
  > m_buf = std::monostate();
};

struct IOLocation {
  std::string path;

  size_t offset, size;
};

class IORequest {
public:
  using Ptr = std::unique_ptr<IORequest>;

  using Id = sched::WorkerPool::JobId;

  using IOJob      = std::unique_ptr<sched::Job<Unit, IOLocation>>;
  using IOComplete = std::function<void(IORequest&)>;
  
  static Ptr read_file(const std::string& path, size_t offset = 0, size_t sz = 0);

  sched::IJob *job();

  // Returns the result of the IO
  //   - Can be called ONLY after completed() == true
  IOBuffer& result();

  bool completed() const;

  // Calls 'fn' upon IO completion in a worker thread
  IORequest& onCompleted(IOComplete fn);

private:
  friend ResourceManager;

  Unit performIo(IOLocation loc);

  IORequest() = default;

  IOJob m_job;
  Id m_id = sched::WorkerPool::InvalidJob;

  IOBuffer m_result;
  IOComplete m_complete;
};


}

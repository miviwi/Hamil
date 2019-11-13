#include <os/thread.h>

#include <win32/thread.h>
#include <sysv/thread.h>

#include <config>

#include <new>

#include <cassert>
#include <cstdlib>
#include <cstring>

namespace os {

struct ThreadData {
  Thread::Id id = Thread::InvalidId;

  Thread::OnExit on_exit;

  u8 storage[1];
};

Thread::Id Thread::current_thread_id()
{
  return InvalidId;
}

Thread *Thread::alloc()
{
#if __win32
  assert(0 && "win32::Thread unimplemented!");
#elif __sysv
  return new sysv::Thread();
#else
#  error "unknown platform"
#endif
}

Thread::Thread(size_t storage_sz) :
  m_data(nullptr)
{
  size_t data_sz = sizeof(ThreadData) - sizeof(ThreadData::storage) + storage_sz;

  m_data = (ThreadData *)malloc(data_sz);
  new(m_data) ThreadData();

  memset(storage(), 0, storage_sz);
}

Thread::~Thread()
{
  if(refs() > 1) return;
 
  if(m_data) {
    m_data->~ThreadData();
    free(m_data);
  }
}

Thread& Thread::create(Fn fn, bool suspended)
{
  doCreate(fn, suspended);

  return *this;
}

Thread& Thread::onExit(OnExit on_exit)
{
  m_data->on_exit = on_exit;

  return *this;
}

void *Thread::storage()
{
  return m_data->storage;
}

const void *Thread::storage() const
{
  return m_data->storage;
}

}

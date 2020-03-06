#include <res/io.h>

#include <math/util.h>
#include <os/file.h>

#include <tuple>
#include <numeric>

namespace res {

IOBuffer::IOBuffer(void *ptr, size_t sz) :
  m_ptr(ptr), m_sz(sz)
{
}

IOBuffer::IOBuffer(os::FileView::Ptr view) :
  m_ptr(view->get()), m_sz(view->size()),
  m_buf(view)
{
}

IOBuffer::IOBuffer(const IOBuffer& other) :
  m_ptr(other.m_ptr), m_sz(other.m_sz),
  m_buf(other.m_buf)
{
}

IOBuffer& IOBuffer::operator=(const IOBuffer& other)
{
  m_ptr = other.m_ptr;
  m_sz  = other.m_sz;

  switch(other.m_buf.index()) {
  case FileView:     m_buf.emplace<FileView>(std::get<FileView>(other.m_buf)); break;
  case MemoryBuffer: m_buf.emplace<MemoryBuffer>(std::get<MemoryBuffer>(other.m_buf)); break;
  }

  return *this;
}

IOBuffer& IOBuffer::operator=(IOBuffer&& other)
{
  std::swap(m_ptr, other.m_ptr);
  std::swap(m_sz, other.m_sz);

  switch(other.m_buf.index()) {
  case FileView:
    m_buf.emplace<FileView>(std::get<FileView>(other.m_buf));
    break;

  case MemoryBuffer:
    m_buf.emplace<MemoryBuffer>(std::move(
      std::get<MemoryBuffer>(other.m_buf)
    ));
    break;
  }

  return *this;
}

IOBuffer IOBuffer::make_memory_buffer(size_t sz)
{
  IOBuffer self;

  auto& buf = self.m_buf.emplace<MemoryBuffer>(new byte[sz]);

  self.m_ptr = buf.get();
  self.m_sz  = sz;

  return self;
}

void *IOBuffer::get()
{
  return m_ptr;
}

size_t IOBuffer::size() const
{
  return m_sz;
}

IOBuffer::operator bool() const
{
  return m_ptr;
}

void IOBuffer::release()
{
  m_ptr = nullptr;
  m_sz = 0;

  m_buf.emplace<Unknown>();
}

IORequest::Ptr IORequest::read_file(const std::string& path, size_t offset, size_t sz)
{
  IOLocation loc;
  IORequest::Ptr req(new IORequest());

  loc.path = path;
  loc.offset = offset;
  loc.size = sz;

  req->m_job.reset(new sched::Job<Unit, IOLocation>(
    std::bind(&IORequest::performIo, req.get(), std::placeholders::_1),
    std::make_tuple(loc)
  ));

  return req;
}

sched::IJob *IORequest::job()
{
  return m_job.get();
}

IOBuffer& IORequest::result()
{
  return m_result;
}

bool IORequest::completed() const
{
  return m_job->done();
}

IORequest& IORequest::onCompleted(IOComplete fn)
{
  m_complete = fn;

  return *this;
}

Unit IORequest::performIo(IOLocation loc)
{
  static constexpr size_t FileSizeMax = std::numeric_limits<os::File::Size>::max();

  auto f = os::File::alloc();
  f().open(loc.path.data(), os::File::Read, os::File::ShareRead, os::File::OpenExisting);

  size_t sz = loc.size ? loc.size : f().size();  // Read the whole file if 'size'
                                                 //   wasn't specified
  m_result = IOBuffer::make_memory_buffer(sz);

  f->seek(os::File::SeekBegin, loc.offset);
  do {  // Make sure to read as much requested data as possible
    auto read = f().read(m_result.get(), saturate<os::File::Size>(sz));
    sz -= read;
  } while(sz >= FileSizeMax &&
    f().seekOffset() < f().size() /* false when => requested_size > file_size */ );

  if(m_complete) m_complete(*this);

  return {};
}

}

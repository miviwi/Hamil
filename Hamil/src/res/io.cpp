#include <res/io.h>

#include <tuple>

namespace res {

IOBuffer::IOBuffer(void *ptr, size_t sz) :
  m_ptr(ptr), m_sz(sz)
{
}

IOBuffer::IOBuffer(win32::FileView view) :
  m_ptr(view.get()), m_sz(view.size()),
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
  // Do some clean up
  m_buf = std::monostate();

  m_ptr = other.m_ptr;
  m_sz  = other.m_sz;

  switch(other.m_buf.index()) {
  case FileView:     m_buf.emplace<win32::FileView>(std::get<win32::FileView>(other.m_buf)); break;
  case MemoryBuffer: m_buf.emplace<MemoryBufferPtr>(std::get<MemoryBufferPtr>(other.m_buf)); break;
  }

  return *this;
}

IOBuffer& IOBuffer::operator=(IOBuffer&& other)
{
  std::swap(m_ptr, other.m_ptr);
  std::swap(m_sz, other.m_sz);

  m_buf = std::monostate();

  switch(other.m_buf.index()) {
  case FileView:
    m_buf.emplace<win32::FileView>(std::get<win32::FileView>(other.m_buf));
    break;

  case MemoryBuffer:
    m_buf.emplace<MemoryBufferPtr>(std::move(
      std::get<MemoryBufferPtr>(other.m_buf)
    ));
    break;
  }
  other.m_buf = std::monostate();

  return *this;
}

IOBuffer IOBuffer::make_memory_buffer(size_t sz)
{
  IOBuffer self;

  auto& buf = self.m_buf.emplace<MemoryBufferPtr>(new byte[sz]);

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

  m_buf = std::monostate();
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
  win32::File f(loc.path.data(), win32::File::Read, win32::File::OpenExisting);
  size_t sz = loc.size ? loc.size : f.size();

  m_result = IOBuffer::make_memory_buffer(sz);

  f.seek(win32::File::SeekBegin, (long)loc.offset);
  f.read(m_result.get(), (win32::File::Size)sz);

  if(m_complete) m_complete(*this);

  return {};
}

}
#include <memory>
#include <os/file.h>
#include <sysv/file.h>

#include <cassert>
#include <cstdlib>
#include <cstring>

#include <config>

namespace os {

File::Ptr File::alloc()
{
#if __win32
  assert(0 && "win32::File unimplemented!");   // TODO: implement
#elif __sysv
  return File::Ptr(sysv::File::alloc());
#else
#  error "unkown platform"
#endif

  return File::Ptr();     // Unreachable (silence warnings)
}

void File::destroy(File *f)
{
#if __win32
  assert(0 && "win32::File unimplemented!");   // TODO: implement
#elif __sysv
  sysv::File::destroy((sysv::File *)f);
#else
#  error "unkown platform"
#endif
}

File::File() :
  m_open(false)
{
}

File::~File()
{
  if(deref()) return;
}

File& File::open(const char *path, Access access, Share share, OpenMode mode)
{
  doOpen(path, access, share, mode);
  m_open = true;

  return *this;
}
  
bool File::isOpen() const
{
  return m_open;
}

FileView::Ptr File::map(Protect prot, size_t offset, size_t size, const char *name)
{
#if __win32
  assert(0 && "win32::File::map unimplemented!");
#elif __sysv
  auto view = FileView::Ptr(new sysv::FileView(this, size, name));
#else
#  error "unknown platform"
#endif

  view->m_ptr = view->doMap(prot, offset);

  return view;
}

FileView::Ptr File::map(Protect prot, const char *name)
{
  return map(prot, 0, size(), name);
}

FileView::FileView(File *file, size_t size, const char *name) :
  m_ptr(nullptr), m_file(file), m_size(size)
{
  m_file->ref();
}

FileView::~FileView()
{
  m_file->deref();
}

void *FileView::get() const
{
  assert(m_ptr && "attempted to get() a pointer to an invalid FileView!");

  return m_ptr;
}

u8& FileView::operator[](size_t offset)
{
  auto p = get<u8>();

  assert(offset < m_size && "attempted to index a FileView beyound it's mapped range!");

  return p[offset];
}

size_t FileView::size() const
{
  return m_size;
}

void FileView::flush(File::Size size)
{
  assert(m_ptr && "attempted to flush() an unmapped FileView!");
  assert(size <= m_size && "attempted to flush() with a size > the mapped range's!");

  doFlush(size ? size : m_size);
}

File *FileView::origin() const
{
  return m_file;
}

void FileQuery::destroy(FileQuery *q)
{
#if __win32
  assert(0 && "win32::FileQuery unimplemented!");
#elif __sysv
  sysv::FileQuery::destroy((sysv::FileQuery *)q);
#else
#  error "unknown platform"
#endif
}

FileQuery::~FileQuery()
{
  if(deref()) return;
}

FileQuery::Ptr FileQuery::null()
{
  return FileQuery::Ptr();
}

FileQuery::Ptr FileQuery::open(const char *path)
{
#if __win32
  assert(0 && "win32::FileQuery unimplemented!");
#elif __sysv
  auto q = FileQuery::Ptr(sysv::FileQuery::alloc());

  q->doOpen(path);

  return q;
#else
#  error "unknown platform"
#endif

  return null();
}

bool current_working_directory(const char *dir)
{
#if __win32
  win32::current_working_directory(dir);
#elif __sysv
  sysv::current_working_directory(dir);
#else
#  error "unknown platform"
#endif

  return false;
}

}

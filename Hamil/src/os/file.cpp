#include <memory>
#include <os/file.h>
#include <sysv/file.h>

#include <cassert>
#include <cstdlib>
#include <cstring>

#include <config>

namespace os {

struct FileData {
  u8 storage[1];
};

File::Ptr File::create()
{
#if __win32
  assert(0 && "win32::File unimplemented!");   // TODO: implement
#elif __sysv
  return File::Ptr(new sysv::File());
#else
#  error "unkown platform"
#endif

  return File::Ptr();     // Unreachable (silence warnings)
}

File::File(size_t storage_sz) :
  m_open(false),
  m_data(nullptr)
{
  size_t data_sz = sizeof(FileData) - sizeof(FileData::storage) + storage_sz;

  m_data = (FileData *)malloc(data_sz);
  new(m_data) FileData();

  memset(m_data->storage, 0, storage_sz);
}

File::~File()
{
  if(refs() > 1) return;

  if(m_data) {
    m_data->~FileData();
    free(m_data);
  }
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

  view->doMap(prot, offset);

  return view;
}

FileView::Ptr File::map(Protect prot, const char *name)
{
  return map(prot, 0, size(), name);
}

void *File::storage()
{
  return m_data->storage;
}

const void *File::storage() const
{
  return m_data->storage;
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

void FileQuery::deleter(FileQuery *q)
{
#if __win32
  assert(0 && "win32::FileQuery unimplemented!");
#elif __sysv
  sysv::FileQuery::destroy((sysv::FileQuery *)q);
#else
#  error "unknown platform"
#endif
}

FileQuery::Ptr FileQuery::null()
{
  return FileQuery::Ptr(nullptr, &FileQuery::deleter);
}

FileQuery::Ptr FileQuery::open(const char *path)
{
#if __win32
  assert(0 && "win32::FileQuery unimplemented!");
#elif __sysv
  auto q = FileQuery::Ptr(sysv::FileQuery::alloc(), &FileQuery::deleter);

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

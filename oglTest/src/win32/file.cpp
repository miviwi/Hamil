#include <win32/file.h>

#include <Windows.h>

namespace win32 {

static const DWORD p_creation_disposition_table[] = {
  CREATE_ALWAYS,
  CREATE_NEW,
  OPEN_ALWAYS,
  OPEN_EXISTING,
  TRUNCATE_EXISTING,
};

File::File(const char *path, Access access, Share share, OpenMode open) :
  m_access(access)
{
  DWORD desired_access = 0;
  desired_access |= access & Read    ? GENERIC_READ    : 0;
  desired_access |= access & Write   ? GENERIC_WRITE   : 0;
  desired_access |= access & Execute ? GENERIC_EXECUTE : 0;

  DWORD share_mode = 0;
  share_mode |= share & ShareRead  ? FILE_SHARE_READ  : 0;
  share_mode |= share & ShareWrite ? FILE_SHARE_WRITE : 0;

  DWORD creation_disposition = p_creation_disposition_table[open];

  m = CreateFileA(path, desired_access, share_mode, nullptr,
                  creation_disposition, FILE_ATTRIBUTE_NORMAL, nullptr);
}

File::File(const char *path, Access access) :
  File(path, access, ShareRead, OpenAlways)
{
}

File::~File()
{
  if(m) CloseHandle(m);
}

File::Size File::read(void *buf, Size sz)
{
  DWORD num_read = 0;
  ReadFile(m, buf, sz, &num_read, nullptr);

  return num_read;
}

File::Size File::write(const void *buf, Size sz)
{
  DWORD num_written = 0;
  WriteFile(m, buf, sz, &num_written, nullptr);

  return num_written;
}

bool File::flush()
{
  return FlushFileBuffers(m) == TRUE;
}

FileView File::map(Protect protect, const char *name)
{
  return map(protect, 0, 0, name);
}

FileView File::map(Protect protect, size_t offset, Size size, const char *name)
{
  DWORD flprotect = 0;
  if(protect == ProtectRead) {
    flprotect = PAGE_READONLY;
  } else if(protect == ProtectReadWrite) {
    flprotect = PAGE_READWRITE;
  } else if(protect == ProtectExecuteRead) {
    flprotect = PAGE_EXECUTE_READ;
  } else if(protect == ProtectExecuteReadWrite) {
    flprotect = PAGE_EXECUTE_READWRITE;
  }
     
  auto mapping = CreateFileMappingA(m, nullptr, flprotect, 0, 0, name);
  return FileView(mapping, m_access, offset, size);
}

FileView::FileView(const FileView& other) :
  m(other.m), m_ptr(other.m_ptr), m_ref(other.m_ref)
{
  *m_ref++;
}

FileView::~FileView()
{
  *m_ref--;
  if(*m_ref) return;

  UnmapViewOfFile(m_ptr);
  CloseHandle(m);
}

void *FileView::get() const
{
  return m_ptr;
}

void FileView::flush(File::Size sz)
{
  FlushViewOfFile(m_ptr, sz);
}

uint8_t& FileView::operator[](size_t offset)
{
  auto ptr = (uint8_t *)m_ptr;
  return ptr[offset];
}

FileView::FileView(void *mapping, File::Access access, size_t offset, File::Size size) :
  m(mapping), m_ref(new unsigned(1))
{
  DWORD desired_access = 0;
  desired_access |= access & File::Read    ? FILE_MAP_READ    : 0;
  desired_access |= access & File::Write   ? FILE_MAP_WRITE   : 0;
  desired_access |= access & File::Execute ? FILE_MAP_EXECUTE : 0;

  m_ptr = MapViewOfFile(m, desired_access, offset>>32, offset&0xFFFFFFFF, size);
}

}
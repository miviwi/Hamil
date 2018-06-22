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
  if(m == INVALID_HANDLE_VALUE) throw FileOpenError(GetLastError());

  DWORD size_low = 0, size_high = 0;
  size_low = GetFileSize(m, &size_high);

  m_sz = dword_combine2(size_high, size_low);
}

File::File(const char *path, Access access, OpenMode open) :
  File(path, access, ShareRead, open)
{
}

File::File(const char *path, Access access) :
  File(path, access, ShareRead, OpenAlways)
{
}

File::~File()
{
  if(m) CloseHandle(m);
}

size_t File::size() const
{
  return m_sz;
}

File::Size File::read(void *buf, Size sz)
{
  DWORD num_read = 0;
  ReadFile(m, buf, sz, &num_read, nullptr);

  return num_read;
}

File::Size File::read(void *buf)
{
  return read(buf, (Size)size());
}

File::Size File::write(const void *buf, Size sz)
{
  DWORD num_written = 0;
  WriteFile(m, buf, sz, &num_written, nullptr);

  return num_written;
}

void File::seek(Seek seek, long offset) const
{
  DWORD move_method = 0;
  switch(seek) {
  case SeekBegin:   move_method = FILE_BEGIN; break;
  case SeekCurrent: move_method = FILE_CURRENT; break;
  case SeekEnd:     move_method = FILE_END; break;
  }

  SetFilePointer(m, offset, nullptr, move_method);
}

bool File::flush()
{
  return FlushFileBuffers(m) == TRUE;
}

FileView File::map(Protect protect, const char *name)
{
  return map(protect, 0, 0, name);
}

FileView File::map(Protect protect, size_t offset, size_t size, const char *name)
{
  DWORD flprotect = 0;
  switch(protect) {
  case ProtectRead:             flprotect = PAGE_READONLY; break;
  case ProtectReadWrite:        flprotect = PAGE_READWRITE; break;
  case ProtectExecuteRead:      flprotect = PAGE_EXECUTE_READ; break;
  case ProtectExecuteReadWrite: flprotect = PAGE_EXECUTE_READWRITE; break;
  }

  auto mapping = CreateFileMappingA(m, nullptr, flprotect, dword_high(size), dword_low(size), name);
  if(!mapping) throw MappingCreateError(GetLastError());

  return FileView(mapping, protect, offset, size);
}

FileView::~FileView()
{
  if(deref()) return;

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

FileView::FileView(void *mapping, File::Protect access, size_t offset, size_t size) :
  m(mapping)
{
  DWORD desired_access = 0;
  desired_access |= access & File::ProtectRead    ? FILE_MAP_READ    : 0;
  desired_access |= access & File::ProtectWrite   ? FILE_MAP_WRITE   : 0;
  desired_access |= access & File::ProtectExecute ? FILE_MAP_EXECUTE : 0;

  m_ptr = MapViewOfFile(m, desired_access, dword_high(offset), dword_low(offset), size);
  if(!m_ptr) throw File::MapFileError(GetLastError());
}

const char *File::Error::errorString() const
{
  switch(what) {
  case ERROR_FILE_NOT_FOUND: return "FileNotFound";
  case ERROR_FILE_INVALID:   return "FileInvalid";
  case ERROR_ACCESS_DENIED:  return "AccessDenied";
  }

  return "<unknown-error>";
}

}
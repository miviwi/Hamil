#include <win32/file.h>

#include <config>

#if __win32
#  include <Windows.h>
#else
#  define DWORD int
#  define INVALID_HANDLE_VALUE (void *)(intptr_t)-1
#endif

#include <cassert>
#include <cstring>
#include <cwchar>

namespace win32 {

static const DWORD p_creation_disposition_table[] = {
#if __win32
  CREATE_ALWAYS,
  CREATE_NEW,
  OPEN_ALWAYS,
  OPEN_EXISTING,
  TRUNCATE_EXISTING,
#else
  0,
  0,
  0,
  0,
  0,
#endif
};

File::File(const char *path, Access access, Share share, OpenMode open) :
  m_access(access), m_full_path(nullptr)
{
#if __win32
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

  LARGE_INTEGER size;
  BOOL result = GetFileSizeEx(m, &size);
  if(result != TRUE) throw FileOpenError(GetLastError());

  m_sz = (size_t)size.QuadPart;
#else
  STUB();
#endif
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
  if(refs() > 1) return;

  delete[] m_full_path;
}

size_t File::size() const
{
  return m_sz;
}

const char *File::fullPath() const
{
#if __win32
  // if the full path has already been queried - return it
  if(m_full_path) return m_full_path;

  // otherwise do the query
  FILE_NAME_INFO temp_name_info;
  auto result = GetFileInformationByHandleEx(m, FileNameInfo, &temp_name_info, sizeof(FILE_NAME_INFO));

  assert(!result && "FullPath was less than 2 characters long!");

  // assumming the full path will be more than 2 characters long
  switch(GetLastError()) {
  case ERROR_MORE_DATA: {
    size_t sz = sizeof(FILE_NAME_INFO) + temp_name_info.FileNameLength;
    auto name_info = (FILE_NAME_INFO *)new char[sz];

    // now that we know the size, actually fill the buffer
    memset(name_info, '\0', sz);
    result = GetFileInformationByHandleEx(m, FileNameInfo, name_info, dword_low(sz));

    assert(result && "GetFileInformationByHandle() failed!");

    // have to convert UTF-16 to UTF-8
    mbstate_t state;
    const wchar_t *str = name_info->FileName;
    auto full_path_len = wcsrtombs(nullptr, &str, 0, &state) + 1; // add space '\0'

    m_full_path = new char[full_path_len];
    memset(m_full_path, '\0', full_path_len);
    wcsrtombs(m_full_path, &str, full_path_len, &state); // do the conversion

    delete[] name_info;
    break;
  }

  default: throw GetFileInfoError(GetLastError());
  }

  return m_full_path;
#else
  return "";
#endif
}

File::Size File::read(void *buf, Size sz)
{
  DWORD num_read = 0;
#if __win32
  ReadFile(m, buf, sz, &num_read, nullptr);
#endif

  return num_read;
}

File::Size File::read(void *buf)
{
  return read(buf, (Size)size());
}

File::Size File::write(const void *buf, Size sz)
{
  DWORD num_written = 0;
#if __win32
  WriteFile(m, buf, sz, &num_written, nullptr);
#endif

  return num_written;
}

void File::seek(Seek seek, size_t offset) const
{
#if __win32
  DWORD move_method = 0;
  switch(seek) {
  case SeekBegin:   move_method = FILE_BEGIN; break;
  case SeekCurrent: move_method = FILE_CURRENT; break;
  case SeekEnd:     move_method = FILE_END; break;
  }

  LARGE_INTEGER move_distance;
  move_distance.QuadPart = offset;

  SetFilePointerEx(m, move_distance, nullptr, move_method);
#endif
}

size_t File::seekOffset() const
{
#if __win32
  LARGE_INTEGER file_pointer;
  LARGE_INTEGER distance;
  distance.QuadPart = 0;

  BOOL result = SetFilePointerEx(m, distance, &file_pointer, FILE_CURRENT);
  if(result != TRUE) throw Error(GetLastError());

  return (size_t)file_pointer.QuadPart;
#else
  return 0;
#endif
}

bool File::flush()
{
#if __win32
  return FlushFileBuffers(m) == TRUE;
#else
  return false;
#endif
}

FileView File::map(Protect protect, const char *name)
{
  return map(protect, 0, 0, name);
}

FileView File::map(Protect protect, size_t offset, size_t size, const char *name)
{
#if __win32
  DWORD flprotect = 0;
  switch(protect) {
  case ProtectRead:             flprotect = PAGE_READONLY; break;
  case ProtectPrivate:          flprotect = PAGE_WRITECOPY; break;
  case ProtectReadWrite:        flprotect = PAGE_READWRITE; break;
  case ProtectExecuteRead:      flprotect = PAGE_EXECUTE_READ; break;
  case ProtectExecutePrivate:   flprotect = PAGE_EXECUTE_WRITECOPY; break;
  case ProtectExecuteReadWrite: flprotect = PAGE_EXECUTE_READWRITE; break;

  default: assert(0 && "File::map() invalid value for 'protect'!");
  }

  auto mapping = CreateFileMappingA(m, nullptr, flprotect, dword_high(size), dword_low(size), name);
  if(!mapping) throw MappingCreateError(GetLastError());

  return FileView(*this, mapping, protect, offset, size);
#else
  STUB();

  return FileView(*this, nullptr, ProtectNone, 0, 0);
#endif
}

FileView::~FileView()
{
  if(deref()) return;

  unmap();
}

void *FileView::get() const
{
  return m_ptr;
}

void FileView::flush(File::Size sz)
{
#if __win32
  FlushViewOfFile(m_ptr, sz);
#endif
}

uint8_t& FileView::operator[](size_t offset)
{
  auto ptr = (uint8_t *)m_ptr;
  return ptr[offset];
}

size_t FileView::size() const
{
  return m_size;
}

void FileView::unmap()
{
#if __win32
  UnmapViewOfFile(m_ptr);
  CloseHandle(m);

  m_ptr = nullptr;
  m = nullptr;
#endif
}

FileView::FileView(const File& file, void *mapping, File::Protect access, size_t offset, size_t size) :
  m_file(file), m_size(size), m(mapping)
{
#if __win32
  DWORD desired_access = 0;
  desired_access |= access & File::ProtectRead    ? FILE_MAP_READ    : 0;
  desired_access |= access & File::ProtectWrite   ? FILE_MAP_WRITE   : 0;
  desired_access |= access & File::ProtectExecute ? FILE_MAP_EXECUTE : 0;

  m_ptr = MapViewOfFile(m, desired_access, dword_high(offset), dword_low(offset), size);
  if(!m_ptr) throw File::MapFileError(GetLastError());
#endif
}

const char *File::Error::errorString() const
{
#if __win32
  switch(what) {
  case ERROR_FILE_NOT_FOUND: return "FileNotFound";
  case ERROR_FILE_INVALID:   return "FileInvalid";
  case ERROR_ACCESS_DENIED:  return "AccessDenied";
  }
#endif

  return "<unknown-error>";
}

FileQuery::FileQuery() :
  m(INVALID_HANDLE_VALUE), m_find_data(nullptr)
{
}

FileQuery::FileQuery(const char *path) :
  FileQuery()
{
  openQuery(path);
}

FileQuery::FileQuery(FileQuery&& other) :
  m(other.m), m_find_data(other.m_find_data)
{
  // null out 'other'
  new(&other) FileQuery();
}

FileQuery::~FileQuery()
{
#if __win32
  closeQuery();
  delete m_find_data;
#endif
}

FileQuery& FileQuery::operator=(FileQuery&& other)
{
  this->~FileQuery();

  m = other.m;
  m_find_data = other.m_find_data;

  // null out 'other'
  new(&other) FileQuery();

  return *this;
}

void FileQuery::foreach(IterFn fn)
{
#if __win32
  auto compare = [](const char *a, const char *b) {
    return strncmp(a, b, sizeof(WIN32_FIND_DATAA::cFileName)) == 0;
  };

  auto find_data = (WIN32_FIND_DATAA *)m_find_data;
  do {
    const char *name = find_data->cFileName;

    // don't iterate over current directory (.) and parent directory (..) in queries
    if(compare(name, ".") || compare(name, "..")) continue;

    unsigned attrs = 0;
    attrs |= find_data->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ? IsDirectory : IsFile;

    fn(name, (Attributes)attrs);
  } while(FindNextFileA(m, find_data));
#endif
}

void FileQuery::openQuery(const char *path)
{
#if __win32
  if(!m_find_data) m_find_data = new WIN32_FIND_DATAA();
  m = FindFirstFileExA(path, FindExInfoBasic, m_find_data, FindExSearchNameMatch, nullptr, 0);

  if(m == INVALID_HANDLE_VALUE) throw Error();
#endif
}

void FileQuery::closeQuery()
{
#if __win32
  if(m != INVALID_HANDLE_VALUE) FindClose(m);
#endif
}

bool current_working_directory(const char *dir)
{
#if __win32
  return SetCurrentDirectoryA(dir);
#else
  return false;
#endif
}

}

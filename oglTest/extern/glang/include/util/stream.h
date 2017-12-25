#pragma once

#include <cstdio>
#include <string>

namespace glang {

class InputStream {
public:
  virtual char peek() = 0;
  virtual char next() = 0;

  virtual bool eof() = 0;

  virtual unsigned line() = 0;
  virtual unsigned column() = 0;

  virtual ~InputStream() {}
};

class ConstInputStream : public InputStream {
public:
  ConstInputStream(const char *str_) :
    m_str(str_), m_line(1), m_column(1)
  { }

  virtual char peek() { return *m_str; }
  virtual char next() { updateLine(); return !eof() ? *m_str++ : '\0'; }

  virtual bool eof() { return *m_str == '\0'; }

  virtual unsigned line() { return m_line; }
  virtual unsigned column() { return m_column; }

private:
  void updateLine()
  {
    if(*m_str == '\n') {
      m_column = 1;
      m_line++;
    } else {
      m_column++;
    }
  }

  const char *m_str;
  unsigned m_line, m_column;
};

class OutputStream {
public:
  virtual void next(char ch) = 0;

  void next(const char *str) { while(*str) next(*str++); }
  void next(void *ptr, size_t sz)
  {
    char *p = (char *)ptr;
    while(sz--) next(*p++);
  }

  virtual ~OutputStream() { }
};

class PipeStream : public InputStream {
public:
  PipeStream() : m_pos(0), m_line(1), m_column(1) { }

  InputStream& output() { return *this; }
  OutputStream& input() { return m_obuf; }

  virtual char peek() { return *(m_obuf.buf()+m_pos); }
  virtual char next() { updateLine(); m_pos++; return !eof() ? *(m_obuf.buf()+m_pos-1) : '\0'; }

  virtual bool eof() { return *(m_obuf.buf()+m_pos) == '\0'; }

  virtual unsigned line() { return m_line; }
  virtual unsigned column() { return m_column; }

  virtual ~PipeStream() { }

private:
  struct OutputBuffer : public OutputStream {
  public:
    virtual void next(char ch) { m_buf += ch; }

    const char *buf() const { return m_buf.data(); }

    virtual ~OutputBuffer() { }

  private:
    std::string m_buf;
  } m_obuf;

  void updateLine()
  {
    if(*(m_obuf.buf()+m_pos) == '\n') {
      m_column = 1;
      m_line++;
    } else {
      m_column++;
    }
  }

  size_t m_pos;

  unsigned m_line, m_column;
};

class StreamSplitter : public OutputStream {
public:
  StreamSplitter(OutputStream& a, OutputStream& b) :
    m_a(&a), m_b(&b) { }

  virtual void next(char ch) { m_a->next(ch); m_b->next(ch); }

private:
  OutputStream *m_a;
  OutputStream *m_b;
};

class NullOutputStream : public OutputStream {
public:
  virtual void next(char ch) { }
};

class ConsoleOutputStream : public OutputStream {
public:
  virtual void next(char ch) { putchar(ch); }
};

__declspec(dllexport) std::string fmt(const char *fmt, ...);

}
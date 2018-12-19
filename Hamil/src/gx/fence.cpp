#include <gx/fence.h>

#include <cassert>
#include <cstring>

namespace gx {

Fence::Fence() :
  m(nullptr),
  m_label(nullptr)
{
}

Fence::~Fence()
{
  if(deref() || !m) return;

  glDeleteSync((GLsync)m);
#if !defined(NDEBUG)
  // Seems delete[] doesn't play nice with nullptr?
  if(m_label) delete[] m_label;
#endif
}

Fence& Fence::sync()
{
#if !defined(NDEBUG)
  assert(m_waited && "Fence::sync() called without a previous wait()/block()!");
#endif

  if(m) { // Fence objects are reusable
    glDeleteSync((GLsync)m);
  }

  m = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
#if !defined(NDEBUG)
  if(m_label) glObjectPtrLabel(m, -1, m_label);

  m_waited = false;
#endif

  glFlush();  // Have to flush here to ensure consistency across
              //   threads (and their associated contexts)

  return *this;
}

bool Fence::signaled()
{
  if(!m) return true;

  auto result = glClientWaitSync((GLsync)m, 0, 0);
  if(result == GL_WAIT_FAILED) throw WaitFailedError();

#if !defined(NDEBUG)
  m_waited = true;
#endif

  return result == GL_ALREADY_SIGNALED || result == GL_CONDITION_SATISFIED;
}

Fence::Status Fence::block(u64 timeout)
{
  if(!m) return Invalid;

  auto result = glClientWaitSync((GLsync)m, GL_SYNC_FLUSH_COMMANDS_BIT, timeout);
  if(result == GL_WAIT_FAILED) throw WaitFailedError();

#if !defined(NDEBUG)
  m_waited = true;
#endif

  Status status = Invalid;
  switch(result) {
  case GL_ALREADY_SIGNALED:
  case GL_CONDITION_SATISFIED: status = Signaled; break;
    
  case GL_TIMEOUT_EXPIRED: status = Timeout; break;
  }

  return status;
}

void Fence::wait()
{
  if(!m) return;

  glWaitSync((GLsync)m, 0, GL_TIMEOUT_IGNORED);

#if !defined(NDEBUG)
  m_waited = true;
#endif
}

void Fence::label(const char *lbl)
{
#if !defined(NDEBUG)
  auto len = strlen(lbl) + 1 /* add space for '\0' */;
  if(m_label) delete[] m_label;

  m_label = new char[len]();  // initialize to 0
  memcpy(m_label, lbl, len);
#endif
}

}
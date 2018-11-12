#include <gx/fence.h>

namespace gx {

Fence::Fence() :
  m(nullptr)
{
}

Fence::~Fence()
{
  if(deref() || !m) return;

  glDeleteSync((GLsync)m);
}

Fence& Fence::sync()
{
  if(m) { // Fence objects are reusable
    glDeleteSync((GLsync)m);
  }

  m = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
  glFlush();  // Have to flush here to ensure consistency across
              //   threads (and their associated contexts)

  return *this;
}

bool Fence::signaled()
{
  auto result = glClientWaitSync((GLsync)m, 0, 0);
  if(result == GL_WAIT_FAILED) throw WaitFailedError();

  return result == GL_ALREADY_SIGNALED || result == GL_CONDITION_SATISFIED;
}

Fence::Status Fence::block(u64 timeout)
{
  if(!m) return Invalid;

  auto result = glClientWaitSync((GLsync)m, GL_SYNC_FLUSH_COMMANDS_BIT, timeout);
  if(result == GL_WAIT_FAILED) throw WaitFailedError();

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
}

void Fence::label(const char *lbl)
{
}

}
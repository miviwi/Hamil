#include <win32/conditionvar.h>

#include <config>

#include <utility>

namespace win32 {

ConditionVariable::ConditionVariable()
{
#if __win32
  InitializeConditionVariable(&m_cv);
#endif
}

ConditionVariable::ConditionVariable(ConditionVariable&& other) :
  m_cv(other.m_cv)
{
#if __win32
  other.m_cv = CONDITION_VARIABLE_INIT;
#endif
}

ConditionVariable& ConditionVariable::operator=(ConditionVariable&& other)
{
#if __win32
  m_cv = other.m_cv;
  other.m_cv = CONDITION_VARIABLE_INIT;
#endif

  return *this;
}

bool ConditionVariable::sleep(os::Mutex& mutex_, ulong timeout)
{
#if __win32
  auto& mutex = (win32::Mutex&)mutex_;
  auto result = SleepConditionVariableCS(&m_cv, &mutex.m, timeout);

  return result == TRUE;
#else
  return false;
#endif
}

os::ConditionVariable& ConditionVariable::wake()
{
#if __win32
  WakeConditionVariable(&m_cv);
#endif

  return *this;
}

os::ConditionVariable& ConditionVariable::wakeAll()
{
#if __win32
  WakeAllConditionVariable(&m_cv);
#endif

  return *this;
}

}

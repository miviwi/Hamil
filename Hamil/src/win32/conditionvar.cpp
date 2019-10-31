#include <win32/conditionvar.h>

namespace win32 {

ConditionVariable::ConditionVariable()
{
#if !defined(__linux__)
  InitializeConditionVariable(&m_cv);
#endif
}

ConditionVariable::ConditionVariable(ConditionVariable&& other) :
  m_cv(other.m_cv)
{
#if !defined(__linux__)
  other.m_cv = CONDITION_VARIABLE_INIT;
#endif
}

ConditionVariable::~ConditionVariable()
{
}

ConditionVariable& ConditionVariable::operator=(ConditionVariable&& other)
{
#if !defined(__linux__)
  m_cv = other.m_cv;
  other.m_cv = CONDITION_VARIABLE_INIT;
#endif

  return *this;
}

bool ConditionVariable::sleep(Mutex& mutex, ulong timeout)
{
#if !defined(__linux__)
  auto result = SleepConditionVariableCS(&m_cv, &mutex.m, timeout);

  return result == TRUE;
#else
  return false;
#endif
}

void ConditionVariable::wake()
{
#if !defined(__linux__)
  WakeConditionVariable(&m_cv);
#endif
}

void ConditionVariable::wakeAll()
{
#if !defined(__linux__)
  WakeAllConditionVariable(&m_cv);
#endif
}

}

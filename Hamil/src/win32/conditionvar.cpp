#include <win32/conditionvar.h>

namespace win32 {

ConditionVariable::ConditionVariable()
{
  InitializeConditionVariable(&m_cv);
}

ConditionVariable::~ConditionVariable()
{
}

bool ConditionVariable::sleep(Mutex& mutex, ulong timeout)
{
  auto result = SleepConditionVariableCS(&m_cv, &mutex.m, timeout);

  return result == TRUE;
}

void ConditionVariable::wake()
{
  WakeConditionVariable(&m_cv);
}

void ConditionVariable::wakeAll()
{
  WakeAllConditionVariable(&m_cv);
}

}
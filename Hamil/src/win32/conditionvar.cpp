#include <win32/conditionvar.h>

namespace win32 {

ConditionVariable::ConditionVariable()
{
  InitializeConditionVariable(&m_cv);
}

ConditionVariable::ConditionVariable(ConditionVariable&& other) :
  m_cv(other.m_cv)
{
  other.m_cv = CONDITION_VARIABLE_INIT;
}

ConditionVariable::~ConditionVariable()
{
}

ConditionVariable& ConditionVariable::operator=(ConditionVariable&& other)
{
  m_cv = other.m_cv;
  other.m_cv = CONDITION_VARIABLE_INIT;

  return *this;
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
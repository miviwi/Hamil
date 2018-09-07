#include <util/ref.h>

Ref::Ref() :
  m_ref(new unsigned(1))
{
}

Ref::Ref(const Ref& other) :
  m_ref(other.m_ref)
{
  ref();
}

Ref::~Ref()
{
  if(*m_ref) return;

  delete m_ref;
}

Ref& Ref::operator=(const Ref& other)
{
  m_ref = other.m_ref;
  ref();

  return *this;
}

unsigned Ref::ref()
{
  (*m_ref)++;

  return *m_ref;
}

bool Ref::deref()
{
  (*m_ref)--;

  return *m_ref;
}

unsigned Ref::refs() const
{
  return *m_ref;
}
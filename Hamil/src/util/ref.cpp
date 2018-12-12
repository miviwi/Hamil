#include <util/ref.h>

#include <new>

Ref::Ref() :
  m_ref(nullptr)  // Allocate the ref-counter on copy
{
}

Ref::Ref(const Ref& other) :
  m_ref(other.m_ref)
{
  if(!other.m_ref) {  // Check if this is the first copy
    other.m_ref = m_ref = new unsigned(1);
  }

  ref();
}

Ref::~Ref()
{
  // Don't derefernce the ref-counter if
  //   it hasn't been allocated (the object
  //   was never copied)
  if(!m_ref || *m_ref) return;

  delete m_ref;
}

Ref& Ref::operator=(const Ref& other)
{
  if(m_ref) {
    deref();
    this->~Ref();
  }

  new(this) Ref(other);

  return *this;
}

unsigned Ref::ref()
{
  if(m_ref) {
    (*m_ref)++;
  } else {   // First copy
    m_ref = new unsigned(2);
  }

  return *m_ref;
}

bool Ref::deref()
{
  if(m_ref) {
    (*m_ref)--;
  } else {
    return 0;  // If the ref-counter was never allocated it
               //   means we were the only Ref to this object
  }

  return *m_ref;
}

unsigned Ref::refs() const
{
  // Ref-counter not allocated -> we're the only Ref
  return m_ref ? *m_ref : 1;
}
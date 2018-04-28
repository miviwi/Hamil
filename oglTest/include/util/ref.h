#pragma once

class Ref {
public:
  Ref();
  Ref(const Ref& other);

  Ref& operator=(const Ref& other);

  unsigned ref();
  bool deref();

  unsigned refs() const;

protected:
  ~Ref();

private:
  unsigned *m_ref;
};
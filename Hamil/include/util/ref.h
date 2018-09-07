#pragma once

// Base class for ref-counted objects
//   - must put
//       if(deref()) return;
//     into the destructor so it's body is run
//     only when the ref-count reaches 0
class Ref {
public:
  Ref();
  Ref(const Ref& other);

  Ref& operator=(const Ref& other);

  // Increments the ref-count and returns it
  unsigned ref();
  // Decrements the ref-count, returns 'false'
  //   when it reaches 0
  bool deref();

  // Returns the current ref-count
  unsigned refs() const;

protected:
  // The destructor is protected because derived classes must
  //   declare one which checks the ref-count
  ~Ref();

private:
  unsigned *m_ref;
};
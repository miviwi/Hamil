#pragma once

#include <vector>

namespace glang {

class Vm;
class ObjectManager;
class Object;

class AutoReleasePool {
public:
  AutoReleasePool(Vm *vm);
  ~AutoReleasePool();

  void autorelease(Object *o);

  Object *operator()(Object *o);

private:
  ObjectManager *m_man;
  std::vector<Object *> m_objs;
};

}

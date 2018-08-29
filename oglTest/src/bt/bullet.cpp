#include <bt/bullet.h>
#include <bt/btcommon.h>

namespace bt {

void init()
{
}

void finalize()
{
}

btVector3 to_btVector3(vec3& v)
{
  return { v.x, v.y, v.z };
}

}
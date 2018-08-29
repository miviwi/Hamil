#include <bt/bullet.h>
#include <bt/btcommon.h>

namespace bt {

void init()
{
}

void finalize()
{
}

btVector3 to_btVector3(const vec3& v)
{
  return { v.x, v.y, v.z };
}

vec3 from_btVector3(const btVector3& v)
{
  return vec3((const float *)v);
}

}
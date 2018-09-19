#include <math/util.h>

#include <util/format.h>

namespace math {

std::string to_str(const vec2& v)
{
  return util::fmt("vec2(%.2f, %.2f)", v.x, v.y);
}

std::string to_str(const ivec2& v)
{
  return util::fmt("ivec2(%d, %d)", v.x, v.y);
}

std::string to_str(const vec3& v)
{
  return util::fmt("vec3(%.2f, %.2f, %.2f)", v.x, v.y, v.z);
}

std::string to_str(const vec4& v)
{
  return util::fmt("vec4(%.2f, %.2f, %.2f, %.2f)", v.x, v.y, v.z, v.w);
}

std::string to_str(const mat3& m)
{
  return util::fmt(
    "mat3(%.2f, %.2f, %.2f,\n"
    "     %.2f, %.2f, %.2f,\n"
    "     %.2f, %.2f, %.2f)",
    m.d[0], m.d[1], m.d[2],
    m.d[3], m.d[4], m.d[5],
    m.d[6], m.d[7], m.d[8]
  );
}

std::string to_str(const mat4& m)
{
  return util::fmt(
    "mat4(%.2f, %.2f, %.2f, %.2f,\n"
    "     %.2f, %.2f, %.2f, %.2f,\n"
    "     %.2f, %.2f, %.2f, %.2f,\n"
    "     %.2f, %.2f, %.2f, %.2f)",
    m.d[0], m.d[1], m.d[2], m.d[3],
    m.d[4], m.d[5], m.d[6], m.d[7],
    m.d[8], m.d[9], m.d[10], m.d[11],
    m.d[12], m.d[13], m.d[14], m.d[15]
  );
}

std::string to_str(const quat& q)
{
  return util::fmt("quat(%.2f, %.2f, %.2f, %.2f)", q.x, q.y, q.z, q.w);
}

}
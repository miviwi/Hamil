#include <math/util.h>

#include <util/format.h>

#include <cmath>
#include <immintrin.h>

u32 loadbe_u32(const void *ptr)
{
  union {
    i32 i;
    u32 u;
  };

  i = _loadbe_i32(ptr);
  return u;
}

std::vector<float> gaussian_kernel(int r)
{
  constexpr float NormFactor = 0.39894f;

  const int kernel_1d_size = r*2 + 1;

  const float sigma      = (float)(r+1) * 2.0f;
  const float inv_sigma  = 1.0f / sigma;
  const float sigma2     = sigma*sigma;
  const float inv_sigma2 = 1.0f / sigma2;

  auto norm_pdf = [=](float x) -> float {
    float neghalf_x2 = -0.5f * (x*x);

    return NormFactor * expf(neghalf_x2 * inv_sigma2) * inv_sigma;
  };

  std::vector<float> kernel(kernel_1d_size);
  float Z = 0.0f;
  for(int i = 0; i <= r; i++) {
    float n = norm_pdf((float)i);

    kernel[r + i] = kernel[r - i] = n;
  }

  for(int i = 0; i < kernel_1d_size; i++) Z += kernel[i];
  Z = 1.0 / (Z*Z);

  kernel.push_back(Z);

  return kernel;
}

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
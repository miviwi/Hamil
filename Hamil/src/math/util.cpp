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

void storebe_u32(u32 v, void *ptr)
{
  union {
    i32 i;
    u32 u;
  };

  u = v;
  _storebe_i32(ptr, i);
}

union Bits {
  float f;
  u32 u;
  i32 i;
};

static constexpr u32 F32Inf  = 0x7F800000u;
static constexpr u32 F32Sign = 0x80000000u;
static constexpr u32 F32MaxN = 0x477FE000u; // Max f16 as f32
static constexpr u32 F32MinN = 0x38800000u; // Min f16 as f32

static constexpr u32 F16Shift     = 13;
static constexpr u32 F16SignShift = 16;

static constexpr u32 F16MulN  = 0x52000000u; // (1<<23) / F32MinN
static constexpr u32 F16MulC  = 0x33800000u; // F32MinN / (1 << (23-F16Shift))
static constexpr u32 F16InfC  = F32Inf >> F16Shift;
static constexpr u32 F16NaN   = (F16InfC + 1) << F16Shift;
static constexpr u32 F16MinC  = F32MinN >> F16Shift;
static constexpr u32 F16MaxC  = F32MaxN >> F16Shift;
static constexpr u32 F16SignC = F32Sign >> F16Shift;

static constexpr u32 F16SubC = 0x003FF;
static constexpr u32 F16NorC = 0x00400;

static constexpr u32 F16MaxD = F16MinC - F16SubC - 1;
static constexpr u32 F16MinD = F16InfC - F16MaxC - 1;

u16 to_f16(float f)
{
  Bits v;
  v.f = f;

  u32 sign = v.u & F32Sign;
  v.u ^= sign;
  sign >>= F16SignShift;

  Bits s;
  s.i = F16MulN;
  s.i = s.f * v.f; // Correct denormals
  v.i ^= (s.i ^ v.i) & -(F32MinN > v.i);
  v.i ^= (F32Inf ^ v.i) & -((F32Inf > v.i) & (v.i > F32MaxN));
  v.i ^= (F16NaN ^ v.i) & -((F16NaN > v.i) & (v.i > F32Inf));

  v.u >>= F16Shift;
  v.i ^= ((v.i - F16MaxD) ^ v.i) & -(v.i > F16MaxC);
  v.i ^= ((v.i - F16MinD) ^ v.i) & -(v.i > F16SubC);

  return (u16)(v.u | sign);
}

float from_f16(u16 h)
{
  Bits v;
  v.u = h;

  u32 sign = v.u & F16SignC;
  v.i ^= sign;
  sign <<= F16SignShift;

  v.i ^= ((v.i + F16MinD) ^ v.i) & -(v.i > F16SubC);
  v.i ^= ((v.i + F16MaxD) ^ v.i) & -(v.i > F16MaxC);

  Bits s;
  s.i = F16MulC;
  s.f *= v.i;

  u32 mask = -(F16NorC > v.i);
  v.i <<= F16Shift;
  v.i ^= (s.i ^ v.i) & mask;
  v.i |= sign;

  return v.f;
}

hvec2 to_f16(const vec2& v)   { return hvec2(to_f16(v.x), to_f16(v.y)); }
vec2 from_f16(const hvec2& v) { return vec2(from_f16(v.x), from_f16(v.y)); }

hvec3 to_f16(const vec3& v)   { return hvec3(to_f16(v.x), to_f16(v.y), to_f16(v.z)); }
vec3 from_f16(const hvec3& v) { return vec3(from_f16(v.x), from_f16(v.y), from_f16(v.z)); }

hvec4 to_f16(const vec4& v)   { return hvec4(to_f16(v.x), to_f16(v.y), to_f16(v.z), to_f16(v.w)); }
vec4 from_f16(const hvec4& v) { return vec4(from_f16(v.x), from_f16(v.y), from_f16(v.z), from_f16(v.w)); }

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
#include <math/brdf.h>

namespace brdf {

BRDF_GGX::EvalResult BRDF_GGX::eval(const vec3& V, const vec3& L, const float alpha) const
{
  // Viewer is behind
  if(V.z <= 0.0f) return std::make_pair(0.0f, 0.0f);

  // Masking
  const float LambdaV = lambda(alpha, V.z);

  // Shadowing
  float G2 = 0.0f;
  if(L.z > 0.0f) {
    const float LambdaL = lambda(alpha, L.z);
    G2 = 1.0f / (1.0f + LambdaV + LambdaL);
  }

  // D
  const vec3 H = (V + L).normalize();
  const vec2 slope = { H.x/H.z, H.y/H.z };

  float D = 1.0f / (1.0f + slope.length2() / alpha / alpha);
  D = D*D;
  D = D / (PIf * alpha*alpha * H.z*H.z*H.z);

  float brdf = D * G2 / 4.0f / V.z;
  float pdf = fabsf(D*H.z / 4.0f / V.dot(H));

  return std::make_pair(brdf, pdf);
}

vec3 BRDF_GGX::sample(const vec3& V, const float alpha, const float U1, const float U2) const
{
  const float phi = 2.0f*PIf * U1;
  const float r = alpha * sqrtf(U2 / (1.0f - U2));

  const vec3 N = vec3(r*cosf(phi), r*sinf(phi), 1.0f).normalize();
  const vec3 L = -V + N*2.0f*N.dot(V);

  return L;
}

float BRDF_GGX::lambda(const float alpha, const float cos_theta) const
{
  const float a = 1.0f / alpha / tanf(acosf(cos_theta));
  if(cos_theta > 1.0f) return 0.0f;

  return 0.5f * (-1.0f + sqrtf(1.0f + 1.0f/a/a));
}

}

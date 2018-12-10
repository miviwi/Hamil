#pragma once

#include <math/geometry.h>

#include <utility>

namespace brdf {

class BRDF_GGX {
public:
  using EvalResult = std::pair<float /* result */, float /* pdf */>;

  EvalResult eval(const vec3& V, const vec3& L, const float alpha) const;

  vec3 sample(const vec3& V, const float alpha, const float U1, const float U2) const;

private:
  float lambda(const float alpha, const float cos_theta) const;

};

}
#pragma once

#include <math/geometry.h>
#include <math/brdf.h>

#include <array>

namespace ltc {

// Linearly transformed cosine
struct LTC {
  // Lobe magnitude
  float magnitude = 1.0f;

  // Average Schlick Fresnel term
  float fresnel = 1.0f;

  // Parametric representation

  float m11 = 1.0f, m22 = 1.0f, m13 = 0.0f;
  vec3 X = vec3::right(), Y = vec3::up(), Z = vec3::forward();

  // Matrix representation (call compute() to calculate it)

  mat3 M;
  mat3 inv_M;
  float det_M;

  // Calculate the matrix representation, it's inverse and det
  void compute();

  using EvalResult = std::pair<float /* result */, float /* pdf */>;

  // Evaluate the LTC for direction 'L'
  EvalResult eval(const vec3& L) const;

  vec3 sample(const float U1, const float U2) const;
};

// PIMPL class
class LTC_CoeffsTableData;

class LTC_CoeffsTable {
public:
  static constexpr uvec2 TableSize = { 64, 64 };
  static constexpr float MinAlpha = 1e-5;

  enum {
    NumSamples = 32,
  };

  using CoeffsArray = std::array<vec4, TableSize.area()>;

  LTC_CoeffsTable();
  LTC_CoeffsTable(const LTC_CoeffsTable& other) = delete;
  LTC_CoeffsTable(LTC_CoeffsTable&& other);
  ~LTC_CoeffsTable();

  LTC_CoeffsTable& fit(const brdf::BRDF_GGX& brdf);

  // LTC transform matrix coefficients
  const CoeffsArray& coeffs1() const;
  const CoeffsArray& coeffs2() const;

private:
  void averageTerms(const brdf::BRDF_GGX& brdf, const vec3& V, float alpha,
    float& norm, float& fresnel, vec3& avg_dir);

  float computeError(const LTC& ltc, const brdf::BRDF_GGX& brdf,
    const vec3& V, float alpha);

  void fitOne(LTC& ltc, const brdf::BRDF_GGX& brdf,
    const vec3& V, float alpha, float eps = 0.05f, bool isotropic = false);

  LTC_CoeffsTableData *m_data;
};

}
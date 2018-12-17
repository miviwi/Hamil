#include <math/ltc.h>
#include <math/neldermead.h>
#include <math/util.h>
#include <math/intrin.h>

#include <util/format.h>

#include <cmath>
#include <cstring>
#include <cstdio>
#include <algorithm>
#include <functional>

namespace ltc {

void LTC::compute()
{
  mat3 coeffs_M = {
    m11,  0.0f, m13,
    0.0f, m22,  0.0f,
    0.0f, 0.0f, 1.0f,
  };

  M = coeffs_M * mat3::from_columns(X, Y, Z);

  inv_M = M.inverse();
  det_M = abs(M.det());
}

LTC::EvalResult LTC::eval(const vec3& L) const
{
  vec3 L_original = (inv_M * L).normalize();
  vec3 L_ = M * L_original;

  float l = L_.length();
  float Jacobian = det_M / (l*l*l);

  float D = 1.0f/PIf * std::max(L_original.z, 0.0f);

  float result = magnitude*D / Jacobian;

  return std::make_pair(result, result / magnitude);
}

vec3 LTC::sample(const float U1, const float U2) const
{
  const float theta = acosf(sqrtf(U1));
  const float phi = 2.0f*PIf * U2;

  vec3 L = M * vec3::from_spherical(theta, phi);

  return L.normalize();
}

class LTC_CoeffsTableData {
public:
  enum {
    NumArrayElems = LTC_CoeffsTable::TableSize.area()
  };

  std::array<intrin::half, NumArrayElems*4> t1;
  std::array<intrin::half, NumArrayElems*4> t2;

  std::array<mat3, NumArrayElems> M;
  std::array<vec2, NumArrayElems> magnitude_fresnel;
  std::array<float, NumArrayElems> sphere;

  // Fill the 'sphere' array with horizon-clipped
  //   sphere form factors
  void generateSphereArray();

  // Fills the 't1' and 't2' coefficient arrays with
  //   data stored in 'M', 'magnitude_fresnel', 'sphere'
  void pack();
};

static float ihemi(float w, float s)
{
  float sins = sinf(s),
    cosw = cos(w);

  float g = asinf(cosf(s) / sinf(w));
  float sins2 = sins*sins;

  auto G = [](float w, float s, float g) {
    return -2.0f*sinf(w)*cosf(s)*cosf(g) + PIf/2.0f - g + sinf(g)*cosf(g);
  };

  auto H = [=](float w, float s, float g) {
    float cosg = cosf(g);
    float cosg2 = cosg*cosg;

    float cosg_over_sins = cosf(g) / sinf(s);
    float angle = asinf(cosg_over_sins);

    return cosf(w) * (cosf(g)*sqrtf(sins2 - cosg2) + sins2*angle);
  };

  if(w >= 0.0f && w <= (PIf/2.0f - s)) {
    return PIf*cosw*sins2;
  } else if(w >= (PIf/2.0f - s) && w < PIf/2.0f) {
    return PIf*cosw*sins2 + G(w, s, g) - H(w, s, g);
  } else if(w >= PIf/2.0f && w < (PIf/2.0f + s)) {
    return G(w, s, g) + H(w, s, g);
  }

  return 0.0f;
}

void LTC_CoeffsTableData::generateSphereArray()
{
  static constexpr int N = LTC_CoeffsTable::TableSize.x;
  for(int j = 0; j < N; j++) {
    for(int i = 0; i < N; i++) {
      const float U1 = (float)i / (float)(N - 1);
      const float U2 = (float)j / (float)(N - 1);

      // z = cos(elevation)
      float z = 2.0f*U1 - 1.0f;

      float len = U2;

      float sigma = asinf(sqrtf(len));
      float omega = acosf(z);

      float value = 0.0f;
      if(sigma > 0.0f) {
        value = ihemi(omega, sigma) / (PIf*len);
      } else {
        value = std::max(z, 0.0f);
      }

      sphere[i + j*N] = value;
    }
  }
}

void LTC_CoeffsTableData::pack()
{
  auto t1_ptr = t1.data(),
    t2_ptr = t2.data();
  for(int i = 0; i < NumArrayElems; i++) {
    const auto& m = M[i];
    auto mag_fresnel = magnitude_fresnel[i];

    auto inv_m = m.inverse();
    inv_m *= 1.0f / inv_m(1, 1);

    auto t1 = vec4(inv_m(0, 0), inv_m(0, 2), inv_m(2, 0), inv_m(2, 2)),
      t2 = vec4(mag_fresnel, 0.0f /* unused */, sphere[i]);

    intrin::stream4_f16(t1, t1_ptr);
    intrin::stream4_f16(t2, t2_ptr);

    t1_ptr += 4;
    t2_ptr += 4;
  }
}

LTC_CoeffsTable::LTC_CoeffsTable()
{
  m_data = new LTC_CoeffsTableData();

  m_data->generateSphereArray();
}

LTC_CoeffsTable::LTC_CoeffsTable(LTC_CoeffsTable&& other) :
  m_data(other.m_data)
{
  other.m_data = nullptr;
}

LTC_CoeffsTable::~LTC_CoeffsTable()
{
  delete m_data;
}

LTC_CoeffsTable& LTC_CoeffsTable::fit(const brdf::BRDF_GGX& brdf)
{
  LTC ltc;

  static constexpr unsigned N = TableSize.x;
  for(int a = N-1; a >= 0; a--) {
    for(int t = 0; t <= N-1; t++) {
      float x = t / float(N-1);
      float ct = 1.0f - x*x;

      // cosf() returns -0.0f when theta == PIf/2
      //   which is undesired, a small bias fixes this
      float theta = std::min(PIf/2.0f - 1e-7f, acosf(ct));

      vec3 V = { sinf(theta), 0.0f, cosf(theta) };

      float roughness = a / float(N-1);
      float alpha = std::max(roughness*roughness, MinAlpha);

      vec3 avg_dir;

      // Fills in 'ltc' and 'avg_dir'
      averageTerms(brdf, V, alpha, ltc.magnitude, ltc.fresnel, avg_dir);

      bool isotropic;

      // 1. First guess for the fit
      if(t == 0) {
        ltc.X = vec3::right();
        ltc.Y = vec3::up();
        ltc.Z = vec3::forward();

        if(a == N-1) { // roughness == 1.0f
          ltc.m11 = 1.0f;
          ltc.m22 = 1.0f;
        } else {       // Use previous roughness
          mat3 prev_M = m_data->M[a + 1 + t*N];

          ltc.m11 = prev_M(0, 0);
          ltc.m22 = prev_M(1, 1);
        }

        ltc.m13 = 0.0f;
        ltc.compute();

        isotropic = true;
      } else {  // Use previous fit as first guess if possible
        // Construct basis
        auto L = avg_dir;
        auto T1 = vec3(L.z, 0.0, -L.x);
        auto T2 = vec3::up();

        ltc.X = T1;
        ltc.Y = T2;
        ltc.Z = L;

        ltc.compute();
        isotropic = false;
      }

      // 2. Fit (refine first guess)
      const float eps = 0.05f;
      fitOne(ltc, brdf, V, alpha, eps, isotropic);

      mat3& M = m_data->M[a + t*N];

      memcpy(M.d, ltc.M.d, sizeof(M));
      m_data->magnitude_fresnel[a + t*N].x = ltc.magnitude;
      m_data->magnitude_fresnel[a + t*N].y = ltc.fresnel;

      // Zero-out useless coefficients
      M(0, 1) = M(1, 0) = M(2, 1) = M(1, 2) = 0.0f;

      auto str = util::fmt("a = %d t = %d\n"
        "roghness = %f (alpha, theta) = (%f, %f)\n"
        "V = %s \t average_dir = %s\n"
        "%s\nmagnitude = %f fresnel = %f\n\n",
        a, t,
        roughness, alpha, theta,
        math::to_str(V), math::to_str(avg_dir),
        math::to_str(M), ltc.magnitude, ltc.fresnel);

      printf(str.data());
    }
  }

  m_data->pack();

  return *this;
}

const LTC_CoeffsTable::CoeffsArray& LTC_CoeffsTable::coeffs1() const
{
  return m_data->t1;
}

const LTC_CoeffsTable::CoeffsArray& LTC_CoeffsTable::coeffs2() const
{
  return m_data->t2;
}

void LTC_CoeffsTable::averageTerms(const brdf::BRDF_GGX& brdf, const vec3& V, float alpha,
  float& norm, float& fresnel, vec3& avg_dir)
{
  norm = fresnel = 0.0f;
  avg_dir = vec3::zero();

  for(int j = 0; j < NumSamples; j++) {
    for(int i = 0; i < NumSamples; i++) {
      auto U1 = ((float)i + 0.5f) / NumSamples;
      auto U2 = ((float)j + 0.5f) / NumSamples;

      // Sample
      vec3 L = brdf.sample(V, alpha, U1, U2);

      auto [eval, pdf] = brdf.eval(V, L, alpha);
      if(pdf == 0.0f) continue;

      float weight = eval / pdf;
      vec3 H = (V + L).normalize();

      float VdotH = std::max(V.dot(H), 0.0f);

      // Accumulate
      norm    += weight;
      fresnel += weight * pow(1.0f - VdotH, 5.0f);
      avg_dir += L * weight;
    }
  }

  // Normalize the results
  norm    /= (float)(NumSamples*NumSamples);
  fresnel /= (float)(NumSamples*NumSamples);

  // Y direction should be == 0 with isotropic BRDFs
  avg_dir.y = 0.0f;
  avg_dir = avg_dir.normalize();
}

float LTC_CoeffsTable::computeError(const LTC& ltc, const brdf::BRDF_GGX& brdf,
  const vec3& V, float alpha)
{
  double error = 0.0;

  // Returns the error with Multiple Importance Sampling weight
  auto calc_error = [](float brdf_pdf, float brdf_eval, float ltc_pdf, float ltc_eval) -> double {
    double e = fabs(brdf_eval - ltc_eval);
    e = e*e*e;

    return e / (brdf_pdf + ltc_pdf);
  };

  static constexpr auto NumSamples2 = (double)(NumSamples*NumSamples);

  for(int j = 0; j < NumSamples; j++) {
    for(int i = 0; i < NumSamples; i++) {
      auto U1 = ((float)i + 0.5f) / NumSamples;
      auto U2 = ((float)j + 0.5f) / NumSamples;

      {      // LTC importance sampling
        vec3 L = ltc.sample(U1, U2);

        auto [brdf_eval, brdf_pdf] = brdf.eval(V, L, alpha);
        auto [ltc_eval, ltc_pdf] = ltc.eval(L);

        error += calc_error(brdf_pdf, brdf_eval, ltc_pdf, ltc_eval);
      }

      {     // BRDF importance sampling
        vec3 L = brdf.sample(V, alpha, U1, U2);

        auto [brdf_eval, brdf_pdf] = brdf.eval(V, L, alpha);
        auto [ltc_eval, ltc_pdf] = ltc.eval(L);

        error += calc_error(brdf_pdf, brdf_eval, ltc_pdf, ltc_eval);
      }
    }
  }

  return (float)(error / NumSamples2);
}

void LTC_CoeffsTable::fitOne(LTC& ltc, const brdf::BRDF_GGX& brdf,
  const vec3& V, float alpha, float eps, bool isotropic)
{
  float start[] = { ltc.m11, ltc.m22, ltc.m13 };
  float result[3];

  auto update = [&](LTC& ltc, const float *params) {
    float m11 = std::max(params[0], 1e-7f);
    float m22 = std::max(params[1], 1e-7f);
    float m13 = params[2];

    if(isotropic) {
      ltc.m11 = ltc.m22 = m11;
      ltc.m13 = 0.0f;
    } else {
      ltc.m11 = m11;
      ltc.m22 = m22;
      ltc.m13 = m13;
    }

    ltc.compute();
  };

  auto get_error = std::bind(&LTC_CoeffsTable::computeError, this, std::placeholders::_1, brdf, V, alpha);

  float error = nedler_mead<3>(result, start, eps, 1e-5f, 100, [&](const float *params) -> float {
    update(ltc, params);
    return get_error(ltc);
  });

  update(ltc, result);
}

}
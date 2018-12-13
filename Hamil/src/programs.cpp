#include <math/geometry.h>

#include <gx/program.h>

struct ProgramUniforms : gx::Uniforms {
  Name program;

  Sampler uTex;
  float uExposure;
};

struct ForwardUniforms : gx::Uniforms {
  Name forward;

  Sampler uDiffuseTex;
  float uExposure;

  Sampler uShadowMap;

  Sampler uGaussianKernel;
  Sampler uLTC_Coeffs;

  int uObjectConstantsOffset;
};

struct ShadowUniforms : gx::Uniforms {
  Name rendermsm;

  int uObjectConstantsOffset;
};

struct AoUniforms : gx::Uniforms {
  Name ao;

  mat4 uInverseView;
  mat4 uProjection;
  vec4 uProjInfo;

  Sampler uEnvironment;
  Sampler uDepth;
  Sampler uNormal;
  Sampler uNoise;

  float uRadius;
  float uRadius2;
  float uNegInvRadius2;
  float uBias;
  float uBiasBoostFactor;
  float uNear;

  vec3 uLightPos;
};

struct SkyboxUniforms : gx::Uniforms {
  Name skybox;

  mat4 uView;
  mat4 uProjection;

  Sampler uEnvironmentMap;
  float uExposure;
};

struct CompositeUniforms : gx::Uniforms {
  Name composite;

  Sampler uUi;
  Sampler uScene;
  Sampler uAo;

  bool uAoEnabled;
};
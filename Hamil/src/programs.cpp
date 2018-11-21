#include <math/geometry.h>

#include <gx/program.h>

struct ProgramUniforms : gx::Uniforms {
  Name program;

  Sampler uTex;
  float uExposure;
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
  float uBias;

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
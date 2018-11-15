#include <math/geometry.h>

#include <gx/program.h>

struct ProgramUniforms : gx::Uniforms {
  Name program;

  Sampler uTex;
  float uExposure;
};

struct AoUniforms : gx::Uniforms {
  Name ao;

  mat4 uProjection;

  Sampler uPosition;
  Sampler uNormal;
  Sampler uNoise;
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
};
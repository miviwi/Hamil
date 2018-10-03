#include <math/geometry.h>

#include <gx/program.h>

struct ProgramUniforms : gx::Uniforms {
  Name program;

  mat4 uModelView;
  mat4 uProjection;
  mat3 uNormal;

  vec4 uCol;
  vec4 uLightPosition;
};

struct TexUniforms : gx::Uniforms {
  Name tex;

  mat4 uModelView;
  mat4 uProjection;
  mat3 uNormal;
  mat4 uTexMatrix;

  Sampler uTex;
};

struct SkyboxUniforms : gx::Uniforms {
  Name skybox;

  mat4 uView;
  mat4 uProjection;

  Sampler uEnvironmentMap;
};

struct CompositeUniforms : gx::Uniforms {
  Name composite;

  Sampler uUi;
  Sampler uScene;
};
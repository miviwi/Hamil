#include <ft/gxuniforms.h>

#include <math/geometry.h>

#include <gx/program.h>

namespace ft {

struct FontUniforms : gx::Uniforms {
    Name font;
    
    Sampler uAtlas;
    mat4 uModelViewProjection;
    vec4 uColor;
};

}
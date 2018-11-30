#include <ek/rendertarget.h>

#include <gx/resourcepool.h>
#include <gx/texture.h>
#include <gx/framebuffer.h>

namespace ek {

RenderTarget RenderTarget::createForwardWithLinearZ(gx::ResourcePool& pool, ivec4 viewport, uint samples)
{
  RenderTarget rt(viewport);

  auto id = pool.create<gx::Framebuffer>();
  auto& fb = pool.get<gx::Framebuffer>(id);

  gx::TextureHandle accumulation, linearz;

  if(samples > 0) {
    accumulation = rt.createTexMultisample(pool, gx::rgb8, samples);
    linearz      = rt.createTexMultisample(pool, gx::r32f, samples);
  } else {
    accumulation = rt.createTex(pool, gx::rgb8);
    linearz      = rt.createTex(pool, gx::r32f);
  }

  fb.use()
    .tex(accumulation.get<gx::Texture2D>(), 0, gx::Framebuffer::Color(0))  /* Accumulation */
    .tex(linearz.get<gx::Texture2D>(), 0, gx::Framebuffer::Color(1))       /* Linear Z */
    .renderbuffer(gx::depth24, gx::Framebuffer::Depth);

  return rt;
}

RenderTarget RenderTarget::createDepthPrepass(gx::ResourcePool& pool, ivec4 viewport, uint samples)
{
  RenderTarget rt(viewport);

  auto id = pool.create<gx::Framebuffer>();
  auto& fb = pool.get<gx::Framebuffer>(id);

  if(samples > 0) {
    fb.use()
      .renderbufferMultisample(samples, gx::depth24, gx::Framebuffer::Depth);
  } else {
    fb.use()
      .renderbuffer(gx::depth24, gx::Framebuffer::Depth);
  }

  return rt;
}

RenderTarget::RenderTarget(ivec4 viewport) :
  m_viewport(viewport)
{
}

gx::TextureHandle RenderTarget::createTexMultisample(gx::ResourcePool& pool, gx::Format fmt, uint samples)
{
  auto id = pool.createTexture<gx::Texture2D>(fmt, gx::Texture::Multisample);
  auto tex = pool.getTexture(id);

  tex.get<gx::Texture2D>().initMultisample(samples, m_viewport.z, m_viewport.w);

  m_texture_ids.append(id);

  return tex;
}

gx::TextureHandle RenderTarget::createTex(gx::ResourcePool& pool, gx::Format fmt)
{
  // TODO: insert return statement here
  auto id = pool.createTexture<gx::Texture2D>(fmt);
  auto tex = pool.getTexture(id);

  tex().init(m_viewport.z, m_viewport.w);

  m_texture_ids.append(id);

  return tex;
}

}
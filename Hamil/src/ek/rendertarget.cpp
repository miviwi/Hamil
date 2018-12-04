#include <ek/rendertarget.h>

#include <gx/resourcepool.h>
#include <gx/texture.h>
#include <gx/framebuffer.h>

#include <cassert>
#include <cstring>
#include "..\..\include\ek\rendertarget.h"

namespace ek {

RenderTargetConfig RenderTargetConfig::depth_prepass(uint samples)
{
  RenderTargetConfig self;

  self.type = DepthPrepass;
  self.samples = samples;

  self.depth = gx::depth24;

  return self;
}

RenderTargetConfig RenderTargetConfig::forward_linearz(uint samples)
{
  RenderTargetConfig self;

  self.type = Forward;
  self.samples = samples;

  self.accumulation.emplace(gx::rgb16f);
  self.linearz.emplace(gx::r32f);
  self.depth = gx::depth24;

  return self;
}

RenderTargetConfig RenderTargetConfig::msm_shadowmap(uint samples)
{
  RenderTargetConfig self;

  self.type = ShadowMap;
  self.samples = samples;

  self.moments.emplace(gx::rgba16);
  self.depth = gx::depth24;

  return self;
}

bool RenderTargetConfig::operator==(const RenderTargetConfig& other) const
{
  return memcmp(this, &other, sizeof(RenderTargetConfig)) == 0;
}

RenderTarget RenderTarget::from_config(const RenderTargetConfig& config, gx::ResourcePool& pool)
{
  RenderTarget rt(config, pool);

  // TODO!
  switch(config.type) {
  case RenderTargetConfig::DepthPrepass: rt.initDepthPrepass(); break;
  case RenderTargetConfig::Forward:      rt.initForward(); break;
  case RenderTargetConfig::ShadowMap:    rt.initShadowMap(); break;

  default: assert(0); // unreachable
  }

  rt.checkComplete();  // Throws CreateError

  return rt;
}

const RenderTargetConfig& RenderTarget::config() const
{
  return m_config;
}

u32 RenderTarget::textureId(TextureType type) const
{
  switch(m_config.type) {
  case RenderTargetConfig::Forward: return forwardTextureId(type);
  }

  return ~0u;
}

void RenderTarget::initForward()
{
  auto& fb = createFramebuffer();

  gx::TextureHandle accumulation, linearz;

  if(m_config.samples > 0) {
    accumulation = createTexMultisample((gx::Format)m_config.accumulation.value(), m_config.samples);
    if(m_config.linearz) {
      linearz = createTexMultisample((gx::Format)m_config.linearz.value(), m_config.samples);
    }
  } else {
    accumulation = createTex((gx::Format)m_config.accumulation.value());
    if(m_config.linearz) {
      linearz = createTex((gx::Format)m_config.linearz.value());
    }
  }

  fb.use()
    .tex(accumulation.get<gx::Texture2D>(), 0, gx::Framebuffer::Color(0))  /* Accumulation */
    .renderbuffer((gx::Format)m_config.depth, gx::Framebuffer::Depth);

  if(m_config.linearz) {
    fb.tex(linearz.get<gx::Texture2D>(), 0, gx::Framebuffer::Color(1));      /* Linear Z */
  }
}

void RenderTarget::initDepthPrepass()
{
  auto& fb = createFramebuffer();

  if(m_config.samples > 0) {
    fb.use()
      .renderbufferMultisample(m_config.samples, (gx::Format)m_config.depth, gx::Framebuffer::Depth);
  } else {
    fb.use()
      .renderbuffer((gx::Format)m_config.depth, gx::Framebuffer::Depth);
  }
}

void RenderTarget::initShadowMap()
{
  auto& fb = createFramebuffer();

  gx::TextureHandle moments;

  if(m_config.samples > 0) {
    moments = createTexMultisample((gx::Format)m_config.moments.value(), m_config.samples);
  } else {
    moments = createTex((gx::Format)m_config.moments.value());
  }

  fb.use()
    .tex(moments.get<gx::Texture2D>(), 0, gx::Framebuffer::Color(0))
    .renderbuffer((gx::Format)m_config.depth, gx::Framebuffer::Depth);
}

void RenderTarget::checkComplete()
{
  auto status = getFramebuffer().status();
  if(status != gx::Framebuffer::Complete) throw CreateError();
}

gx::Framebuffer& RenderTarget::getFramebuffer()
{
  return m_pool->get<gx::Framebuffer>(m_fb_id);
}

u32 RenderTarget::forwardTextureId(TextureType type) const
{
  switch(type) {
  case Accumulation: return m_texture_ids.at(0);
  case LinearZ:      return m_texture_ids.size() > 1 ? m_texture_ids.at(1) : ~0u;
  }

  return ~0u;
}

u32 RenderTarget::shadowMapTexureId(TextureType type) const
{
  switch(type) {
  case Moments: return m_texture_ids.at(0);
  }

  return ~0u;
}

bool RenderTarget::lock() const
{
  bool locked = false;

  return m_in_use.compare_exchange_strong(locked, false);
}

void RenderTarget::unlock() const
{
  m_in_use.store(false);
}

u32 RenderTarget::framebufferId() const
{
  return m_fb_id;
}

RenderTarget::RenderTarget(const RenderTargetConfig& config, gx::ResourcePool& pool) :
  m_config(config),
  m_pool(&pool),
  m_fb_id(~0u),
  m_in_use(false)
{
}

RenderTarget::RenderTarget(const RenderTarget& other) :
  Ref(other),
  m_config(other.m_config),
  m_pool(other.m_pool),
  m_fb_id(other.m_fb_id), m_texture_ids(other.m_texture_ids),
  m_in_use(other.m_in_use.load())
{
}

RenderTarget::~RenderTarget()
{
  if(deref()) return;

  // Release the Framebuffer first
  m_pool->release<gx::Framebuffer>(m_fb_id);

  // ...and then it's Textures
  for(auto& tex_id : m_texture_ids) m_pool->releaseTexture(tex_id);
}

gx::Framebuffer& RenderTarget::createFramebuffer()
{
  m_fb_id = m_pool->create<gx::Framebuffer>();

  return m_pool->get<gx::Framebuffer>(m_fb_id);
}

gx::TextureHandle RenderTarget::createTexMultisample(gx::Format fmt, uint samples)
{
  auto id = m_pool->createTexture<gx::Texture2D>(fmt, gx::Texture::Multisample);
  auto tex = m_pool->getTexture(id);

  tex.get<gx::Texture2D>().initMultisample(samples, m_config.viewport.z, m_config.viewport.w);

  m_texture_ids.append(id);

  return tex;
}

gx::TextureHandle RenderTarget::createTex(gx::Format fmt)
{
  auto id = m_pool->createTexture<gx::Texture2D>(fmt);
  auto tex = m_pool->getTexture(id);

  tex().init(m_config.viewport.z, m_config.viewport.w);

  m_texture_ids.append(id);

  return tex;
}

}
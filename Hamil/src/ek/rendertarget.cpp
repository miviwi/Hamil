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
  RenderTarget rt(config);

  // TODO!
  switch(config.type) {
  case RenderTargetConfig::DepthPrepass: rt.initDepthPrepass(pool); break;
  case RenderTargetConfig::Forward:      rt.initForward(pool); break;
  case RenderTargetConfig::ShadowMap:    rt.initShadowMap(pool); break;

  default: assert(0); // unreachable
  }

  rt.checkComplete(pool);  // Throws CreateError

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

void RenderTarget::initForward(gx::ResourcePool& pool)
{
  auto& fb = createFramebuffer(pool);

  gx::TextureHandle accumulation, linearz;

  if(m_config.samples > 0) {
    accumulation = createTexMultisample(pool, (gx::Format)m_config.accumulation.value(), m_config.samples);
    if(m_config.linearz) {
      linearz = createTexMultisample(pool, (gx::Format)m_config.linearz.value(), m_config.samples);
    }
  } else {
    accumulation = createTex(pool, (gx::Format)m_config.accumulation.value());
    if(m_config.linearz) {
      linearz = createTex(pool, (gx::Format)m_config.linearz.value());
    }
  }

  fb.use()
    .tex(accumulation.get<gx::Texture2D>(), 0, gx::Framebuffer::Color(0))  /* Accumulation */
    .renderbuffer((gx::Format)m_config.depth, gx::Framebuffer::Depth);

  if(m_config.linearz) {
    fb.tex(linearz.get<gx::Texture2D>(), 0, gx::Framebuffer::Color(1));      /* Linear Z */
  }
}

void RenderTarget::initDepthPrepass(gx::ResourcePool& pool)
{
  auto& fb = createFramebuffer(pool);

  if(m_config.samples > 0) {
    fb.use()
      .renderbufferMultisample(m_config.samples, (gx::Format)m_config.depth, gx::Framebuffer::Depth);
  } else {
    fb.use()
      .renderbuffer((gx::Format)m_config.depth, gx::Framebuffer::Depth);
  }
}

void RenderTarget::initShadowMap(gx::ResourcePool& pool)
{
  auto& fb = createFramebuffer(pool);

  gx::TextureHandle moments;

  if(m_config.samples > 0) {
    moments = createTexMultisample(pool, (gx::Format)m_config.moments.value(), m_config.samples);
  } else {
    moments = createTex(pool, (gx::Format)m_config.moments.value());
  }

  fb.use()
    .tex(moments.get<gx::Texture2D>(), 0, gx::Framebuffer::Color(0))
    .renderbuffer((gx::Format)m_config.depth, gx::Framebuffer::Depth);
}

void RenderTarget::checkComplete(gx::ResourcePool& pool)
{
  auto& fb = pool.get<gx::Framebuffer>(m_fb_id);

  auto status = fb.status();
  if(status != gx::Framebuffer::Complete) throw CreateError();
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

RenderTarget::RenderTarget(const RenderTargetConfig& config) :
  m_config(config),
  m_in_use(false)
{
}

RenderTarget::RenderTarget(const RenderTarget& other) :
  m_config(other.m_config),
  m_fb_id(other.m_fb_id), m_texture_ids(other.m_texture_ids),
  m_in_use(other.m_in_use.load())
{
}

RenderTarget::~RenderTarget()
{
  if(deref()) return;

  // TODO!!
  for(auto& tex_id : m_texture_ids) {
  }
}

gx::Framebuffer& RenderTarget::createFramebuffer(gx::ResourcePool& pool)
{
  m_fb_id = pool.create<gx::Framebuffer>();

  return pool.get<gx::Framebuffer>(m_fb_id);
}

gx::TextureHandle RenderTarget::createTexMultisample(gx::ResourcePool& pool, gx::Format fmt, uint samples)
{
  auto id = pool.createTexture<gx::Texture2D>(fmt, gx::Texture::Multisample);
  auto tex = pool.getTexture(id);

  tex.get<gx::Texture2D>().initMultisample(samples, m_config.viewport.z, m_config.viewport.w);

  m_texture_ids.append(id);

  return tex;
}

gx::TextureHandle RenderTarget::createTex(gx::ResourcePool& pool, gx::Format fmt)
{
  auto id = pool.createTexture<gx::Texture2D>(fmt);
  auto tex = pool.getTexture(id);

  tex().init(m_config.viewport.z, m_config.viewport.w);

  m_texture_ids.append(id);

  return tex;
}

}
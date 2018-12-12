#include <ek/rendertarget.h>

#include <gx/resourcepool.h>
#include <gx/texture.h>
#include <gx/framebuffer.h>

#include <cassert>
#include <cstring>

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

RenderTargetConfig RenderTargetConfig::moment_shadow_map(uint samples)
{
  RenderTargetConfig self;

  self.type = ShadowMap;
  self.samples = samples;

  self.moments.emplace(gx::rgba32f);
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
  if(type == Depth) {
    return m_config.depth_texture ? m_texture_ids.back() : ~0u;
  }

  switch(m_config.type) {
  case RenderTargetConfig::Forward:   return forwardTextureId(type);
  case RenderTargetConfig::ShadowMap: return shadowMapTexureId(type);
  }

  return ~0u;
}

void RenderTarget::initForward()
{
  auto& fb = createFramebuffer();

  auto accumulation_label = "t2d" + labelSuffix() + "Accumulation";
  auto linearz_label = "t2d" + labelSuffix() + "LinearZ";

  gx::TextureHandle accumulation, linearz;
  if(m_config.samples > 0) {
    accumulation = createTexMultisample(m_config.accumulation.value(), m_config.samples,
      accumulation_label);
    if(m_config.linearz) {
      linearz = createTexMultisample(m_config.linearz.value(), m_config.samples,
        linearz_label);
    }
  } else {
    accumulation = createTex(m_config.accumulation.value(), accumulation_label);
    if(m_config.linearz) {
      linearz = createTex(m_config.linearz.value(), linearz_label);
    }
  }

  fb.use()
    .tex(accumulation.get<gx::Texture2D>(), 0, gx::Framebuffer::Color(0));  /* Accumulation */

  if(m_config.linearz) {
    fb.tex(linearz.get<gx::Texture2D>(), 0, gx::Framebuffer::Color(1));      /* Linear Z */
  }

  initDepth();
}

void RenderTarget::initDepthPrepass()
{
  auto& fb = createFramebuffer();

  auto label = "rb" + labelSuffix() + "Depth";

  if(m_config.samples > 0) {
    fb.use()
      .renderbufferMultisample(m_config.samples, (gx::Format)m_config.depth, gx::Framebuffer::Depth,
        label.data());
  } else {
    fb.use()
      .renderbuffer((gx::Format)m_config.depth, gx::Framebuffer::Depth, label.data());
  }
}

void RenderTarget::initShadowMap()
{
  auto& fb = createFramebuffer();

  auto label = "t2d" + labelSuffix() + "Moments";

  gx::TextureHandle moments;
  if(m_config.samples > 0) {
    moments = createTexMultisample(m_config.moments.value(), m_config.samples, label);
  } else {
    moments = createTex(m_config.moments.value(), label);
  }

  fb.use()
    .tex(moments.get<gx::Texture2D>(), 0, gx::Framebuffer::Color(0));

  initDepth();
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

u32 RenderTarget::framebufferId() const
{
  return m_fb_id;
}

RenderTarget::RenderTarget(const RenderTargetConfig& config, gx::ResourcePool& pool) :
  m_config(config),
  m_pool(&pool),
  m_fb_id(~0u)
{
}

RenderTarget::RenderTarget(const RenderTarget& other) :
  Ref(other),
  m_config(other.m_config),
  m_pool(other.m_pool),
  m_fb_id(other.m_fb_id), m_texture_ids(other.m_texture_ids)
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

std::string RenderTarget::labelSuffix() const
{
#if !defined(NDEBUG)
  switch(m_config.type) {
  case RenderTargetConfig::DepthPrepass: return "DepthPrepass";
  case RenderTargetConfig::Forward:      return "Forward";
  case RenderTargetConfig::Deferred:     return "Deferred";

  case RenderTargetConfig::LightPropagationVolume: return "LightPropagationVolume";
  case RenderTargetConfig::ShadowMap:              return "ShadowMap";
  case RenderTargetConfig::ReflectiveShadowMap:    return "ReflectiveShadowMap";
  }
#endif

  return "";
}

gx::Framebuffer& RenderTarget::createFramebuffer()
{
  auto label = "fb" + labelSuffix();
  m_fb_id = m_pool->create<gx::Framebuffer>(label.data());

  return m_pool->get<gx::Framebuffer>(m_fb_id);
}

gx::TextureHandle RenderTarget::createTexMultisample(u32 fmt, uint samples, const std::string& label)
{
  auto id = m_pool->createTexture<gx::Texture2D>(label.data(), (gx::Format)fmt, gx::Texture::Multisample);
  auto tex = m_pool->getTexture(id);

  tex.get<gx::Texture2D>().initMultisample(samples, m_config.viewport.z, m_config.viewport.w);

  m_texture_ids.append(id);

  return tex;
}

gx::TextureHandle RenderTarget::createTex(u32 fmt, const std::string& label)
{
  auto id = m_pool->createTexture<gx::Texture2D>(label.data(), (gx::Format)fmt);
  auto tex = m_pool->getTexture(id);

  tex().init(m_config.viewport.z, m_config.viewport.w);

  m_texture_ids.append(id);

  return tex;
}

void RenderTarget::initDepth()
{
  auto& fb = getFramebuffer();


  if(m_config.depth_texture) {
    std::string label = "t2d" + labelSuffix() + "Depth";
    gx::TextureHandle depth;

    if(m_config.samples > 0) {
      depth = createTexMultisample(m_config.depth, m_config.samples, label);
    } else {
      depth = createTex(m_config.depth, label);
    }

    fb.use()
      .tex(depth.get<gx::Texture2D>(), 0, gx::Framebuffer::Depth);
  } else {
    std::string label = "rb" + labelSuffix() + "Depth";

    // Framebuffer::renderbuffer() handles multisampling transparently,
    //   i.e. when at least one render target has MSAA it will automatically
    //   set it for all renderbuffers
    fb.use()
      .renderbuffer((gx::Format)m_config.depth, gx::Framebuffer::Depth, label.data());
  }
}

}
#include <ui/ui.h>
#include <ui/frame.h>
#include <ui/painter.h>
#include <ui/drawable.h>

#include <math/xform.h>
#include <math/util.h>
#include <os/panic.h>

#include <uniforms.h>
#include <gx/pipeline.h>
#include <gx/buffer.h>
#include <gx/vertex.h>
#include <gx/program.h>
#include <gx/fence.h>
#include <gx/renderpass.h>
#include <gx/commandbuffer.h>

#include <algorithm>
#include <memory>

// Uncomment the line below to render the ui without 4xMSAA
//#define UI_FB_NO_MULTISAMPLING 1

namespace ui {

struct UiUniforms : gx::Uniforms {
  Name ui;

  mat4 uModelViewProjection;
  int uType;
  int uImagePage;
  Sampler uFontAtlas;
  Sampler uImageAtlas;
  vec4 uTextColor;
};

static const char *shader_uType_defs = R"DEFS(

const int TypeShape = 0;
const int TypeText  = 1;
const int TypeImage = 2;

)DEFS";

// Must be kept in sync with GLSL!!
enum Shader_uType {
  Shader_TypeShape = 0,
  Shader_TypeText  = 1,
  Shader_TypeImage = 2,
};

static const char *vs_src = R"VTX(

uniform mat4 uModelViewProjection;

uniform int uType;

const float fixed_factor = 1.0 / float(1<<4);

layout(location = 0) in vec2 iPos;
layout(location = 1) in vec4 iColor;
layout(location = 2) in vec2 iUV;

out VertexData {
  vec4 color;
  vec2 uv;
} vertex;

void main() {
  vec2 pos = iPos;
  if(uType != TypeText) pos *= fixed_factor; // TypeText vertices are already normalized

  vertex.uv = iUV;
  vertex.color = vec4(iColor.rgb*iColor.a, iColor.a);

  gl_Position = uModelViewProjection * vec4(pos, 0, 1);
}

)VTX";

static const char *fs_src = R"FRAG(

uniform sampler2D uFontAtlas;

uniform sampler2DArray uImageAtlas;

uniform int uType;
uniform vec4 uTextColor;
uniform float uImagePage;

in VertexData {
  vec4 color;
  vec2 uv;
} fragment;

layout(location = 0) out vec4 color;

vec4 text()
{
  vec4 sample = sampleFontAtlas(uFontAtlas, fragment.uv);
  vec4 color  = uTextColor * sample;

  return vec4(color.rgb*color.a, color.a);
}

vec4 shape()
{
  return fragment.color;    // The per-vertex color was already premultiplied
}                           //   by the vertex shader

vec4 image()
{
  ivec3 atlas_sz = textureSize(uImageAtlas, 0);
  vec4 sample    = texture(uImageAtlas, vec3(fragment.uv / atlas_sz.st, uImagePage));

  return vec4(sample.rgb*sample.a, sample.a);
}

void main() {
  switch(uType) {
    case TypeText:  color = text(); break;
    case TypeShape: color = shape(); break;
    case TypeImage: color = image(); break;

    default: color = vec4(0); break;
  }
}

)FRAG";

void init()
{
  CursorDriver::init();
}

void finalize()
{
}

Ui::Ui(gx::ResourcePool& pool, Geometry geom, const Style& style) :
  m_real_size(geom.size()),
  m_geom(geom), m_style(style),
  m_capture(nullptr),
  m_keyboard(nullptr),
  m_pool(pool),
  m_mempool(512*1024),
  m_framebuffer_tex_id(gx::ResourcePool::Invalid),
  m_framebuffer_id(gx::ResourcePool::Invalid),
  m_program_id(gx::ResourcePool::Invalid),
  m_renderpass_id(gx::ResourcePool::Invalid),
  m_drawable(m_pool),
  m_painter(m_pool),
  m_buf(gx::Buffer::Dynamic),
  m_ind(gx::Buffer::Dynamic, gx::Type::u16),
  m_vtx_id(gx::ResourcePool::Invalid)
{
  m_style.init(m_pool);

#if !defined(UI_FB_NO_MULTISAMPLING)
  m_framebuffer_tex_id = m_pool.createTexture<gx::Texture2D>("t2dUiFramebuffer",
    gx::rgba8, gx::Texture::Multisample);
  m_pool.getTexture<gx::Texture2D>(m_framebuffer_tex_id)
    .initMultisample(4, FramebufferSize.cast<int>());
#else
  m_framebuffer_tex_id = m_pool.createTexture<gx::Texture2D>("t2dUiFramebuffer", gx::rgba8);
  m_pool.getTexture<gx::Texture2D>(m_framebuffer_tex_id)
    .init(FramebufferSize.cast<int>());
#endif

  m_framebuffer_id = m_pool.create<gx::Framebuffer>("fbUi");
  auto& fb = m_pool.get<gx::Framebuffer>(m_framebuffer_id);
  fb.use()
    .tex(m_pool.getTexture<gx::Texture2D>(m_framebuffer_tex_id), 0, gx::Framebuffer::Color(0));

  // TODO: Just panic for now...
  if(!fb.complete()) {
    os::panic("Couldn't create UI Framebuffer!", os::FramebufferError);
  }

  m_program_id = m_pool.create<gx::Program>(gx::make_program("pUi",
    { shader_uType_defs, vs_src }, { ft::Font::frag_shader, shader_uType_defs, fs_src }, U.ui
  ));

  m_renderpass_id = m_pool.create<gx::RenderPass>();
  m_pool.get<gx::RenderPass>(m_renderpass_id)
    .framebuffer(m_framebuffer_id)
    .pipeline(gx::Pipeline(&m_pool)
      .add<gx::Pipeline::Viewport>(0, 0, FramebufferSize.s, FramebufferSize.t)
      .add<gx::Pipeline::Scissor>([](auto& sc) {
          sc.no_test();
      })
      .add<gx::Pipeline::DepthStencil>([](auto& ds) {
          ds.no_depth_test();
      })
      .add<gx::Pipeline::Blend>([](auto& b) {
          b.premult_alpha_blend();
      })
      .add<gx::Pipeline::ClearColor>(transparent().normalize())
    )
    .textures({
      { DrawableManager::TexImageUnit, { m_drawable.atlasId(), m_drawable.samplerId() } },

      // The default font is most likely to be used first
      //   TODO: pack them into a Texture2DArray maybe?
      { ft::TexImageUnit,              { m_style.font->atlasId(), m_drawable.samplerId() } },
    })
    .clearOp(gx::RenderPass::ClearColor);

  m_buf.label("bvUi");
  m_ind.label("biUi");

  m_vtx_id = m_pool.create<gx::IndexedVertexArray>("iaUi",
    VertexPainter::Fmt, m_buf, m_ind);

  m_fence_id = m_pool.create<gx::Fence>("fUiPaintDone");

  m_painter.useVertexArray(m_vtx_id);
}

Ui::~Ui()
{
  for(const auto& frame : m_frames) delete frame;
}

ivec4 Ui::scissorRect(Geometry g)
{
  auto ratio = m_real_size * FramebufferSize.recip();   // == m_real_size / FramebufferSize

  auto pos = g.pos() * ratio,
    size = g.size() * ratio;

  auto gb = pos + size;

  return ivec4{ (int)pos.x, (int)(m_real_size.y - gb.y), (int)size.x, (int)size.y };
}

Ui& Ui::realSize(vec2 real_size)
{
  m_real_size = real_size;
  return *this;
}

Ui& Ui::frame(Frame *f, vec2 pos)
{
  if(!pos.x && !pos.y) pos = m_geom.center();
  
  f->m_geom.x = pos.x; f->m_geom.y = pos.y;
  f->attached(nullptr);

  m_frames.push_back(f);

  return *this;
}

Ui& Ui::frame(Frame *f)
{
  f->attached(nullptr);
  m_frames.push_back(f);

  return *this;
}

Ui& Ui::frame(Frame& f, vec2 pos)
{
  return frame(&f, pos);
}

Ui& Ui::frame(Frame &f)
{
  return frame(&f);
}

void Ui::registerFrame(Frame *frame)
{
  if(!frame->m_name) return;

  auto result = m_names.insert({ frame->m_name, frame });
  auto it = result.first;

  // Frame name collision
  if(!result.second) return;

  // Store the internalized string
  frame->m_name = it->first.data();
}

Frame *Ui::getFrameByName(const std::string& name)
{
  auto f = m_names.find(name);
  return f != m_names.end() ? f->second : nullptr;
}

const Style& Ui::style() const
{
  return m_style;
}

DrawableManager& Ui::drawable()
{
  return m_drawable;
}

gx::ResourcePool::Id Ui::framebufferTextureId()
{
  return m_framebuffer_tex_id;
}

gx::ResourcePool::Id Ui::framebufferId()
{
  return m_framebuffer_id;
}

bool Ui::input(CursorDriver& cursor, const InputPtr& input)
{
  if(auto mouse = input->get<os::Mouse>()) {
    if(mouse->event == os::Mouse::Move && !cursor.visible()) cursor.visible(true);
  }

  // The cursor is outside the Ui, so don't consume the input
  if(!m_geom.intersect(cursor.pos())) return false;

  if(m_keyboard) {
    if(input->get<os::Keyboard>()) return m_keyboard->input(cursor, input);
  } else if(m_capture) {
    return m_capture->input(cursor, input);
  }

  for(auto it = m_frames.begin(); it != m_frames.end(); it++) {
    auto frame = *it;

    // The input was outside this Frame - try the next one
    if(!frame->input(cursor, input)) continue;

    // After the user interacts with a top-level Frame
    //   bring it to the front (i.e. draw it on top of
    //   everything else and pass input to it first)
    m_frames.erase(it);
    m_frames.push_back(frame);
    return true;
  }

  return false;
}

gx::CommandBuffer Ui::paint()
{
  if(m_frames.empty()) return gx::CommandBuffer::begin().end();

  // Have to wait for the previous paint to complete
  m_pool.get<gx::Fence>(m_fence_id).block();

  // Make sure the MemoryPool is clean (previous paint()
  //   leaves behind data)
  m_mempool.purge();

  auto& renderpass = m_pool.get<gx::RenderPass>(m_renderpass_id);
  auto command_buf = gx::CommandBuffer::begin()
    .bindResourcePool(&m_pool)
    .bindMemoryPool(&m_mempool);

  if(m_drawable.prepareDraw()) {
    command_buf.fenceWait(m_drawable.fenceId());
  }

  if(m_repaint || true) {     // TODO: intelligently decide on this somehow?
    // Same as above - clean up after previous paint()
    m_painter.end();

    m_buf.init(sizeof(Vertex), VertexPainter::BufferSize);
    m_ind.init(sizeof(u16), VertexPainter::BufferSize);

    // We've just orphaned the old storage which means,
    //   even if there are in-flight commands which operate
    //   on the buffers we can disregard that as the driver
    //   will take care of synchronization/storage lifetime
    // (Turns out - waiting for vsync with swapBuffers() does
    //   not guarantee the previous commands have executed)
    auto map_flags = gx::Buffer::MapUnsynchronized | gx::Buffer::MapInvalidate;

    auto verts = m_buf.map(gx::Buffer::Write, map_flags);
    auto inds = m_ind.map(gx::Buffer::Write, map_flags);

    m_painter.begin(
      verts.get<Vertex>(), VertexPainter::BufferSize,
      inds.get<u16>(), VertexPainter::BufferSize);

    // Fill the vertex and index buffers
    for(const auto& frame : m_frames) frame->paint(m_painter, m_geom);
  }

  auto projection = xform::ortho(0, 0, FramebufferSize.y, FramebufferSize.x, 0.0f, 1.0f);

  command_buf
    .renderpass(m_renderpass_id)
    .program(m_program_id)
    .uniformSampler(U.ui.uFontAtlas, ft::TexImageUnit)
    .uniformSampler(U.ui.uImageAtlas, DrawableManager::TexImageUnit);

  // 'm_renderpass_id' binds style().font by default
  gx::ResourcePool::Id last_font_id = style().font->atlasId();

  m_painter.doCommands([&,this](VertexPainter::Command cmd)
  {
    auto mvp_handle = m_mempool.alloc<mat4>();
    auto& mvp = *m_mempool.ptr<mat4>(mvp_handle);

    mvp = projection;

    auto prim = cmd.d.p;
    auto num = (u32)cmd.d.num;
    auto base = (u32)cmd.d.base;
    auto offset = (u32)cmd.d.offset;

    const auto& pos = cmd.d.pos;

    switch(cmd.type) {
    case VertexPainter::Primitive:
      command_buf
        .uniformMatrix4x4(U.ui.uModelViewProjection, mvp_handle)
        .uniformInt(U.ui.uType, Shader_TypeShape)
        .drawBaseVertex(prim, m_vtx_id, num, base, offset);
      break;

    case VertexPainter::Text: {
      if(last_font_id != cmd.d.font->atlasId()) {
        auto font_id = cmd.d.font->atlasId();
        auto subpass_id = renderpass.nextSubpassId();
        renderpass.subpass(gx::RenderPass::Subpass()
          .texture(ft::TexImageUnit, font_id, m_drawable.samplerId()));

        command_buf.subpass(subpass_id);

        last_font_id = font_id;
      }

      auto color_handle = m_mempool.alloc<vec4>();
      auto& color = *m_mempool.ptr<vec4>(color_handle);

      color = cmd.d.color.normalize();
      mvp = mvp * xform::translate(pos.x, pos.y, 0);

      command_buf
        .uniformMatrix4x4(U.ui.uModelViewProjection, mvp_handle)
        .uniformInt(U.ui.uType, Shader_TypeText)
        .uniformVector4(U.ui.uTextColor, color_handle)
        .drawBaseVertex(prim, m_vtx_id, num, base, offset);
      break;
    }

    case VertexPainter::Image:
      mvp = mvp * xform::translate(pos.x, pos.y, 0);

      command_buf
        .uniformMatrix4x4(U.ui.uModelViewProjection, mvp_handle)
        .uniformInt(U.ui.uType, Shader_TypeImage)
        .uniformFloat(U.ui.uImagePage, (float)cmd.d.page)
        .drawBaseVertex(prim, m_vtx_id, num, base, offset);
      break;

    case VertexPainter::Pipeline: {
      auto subpass_id = renderpass.nextSubpassId();
      renderpass.subpass(gx::RenderPass::Subpass()
        .pipeline(cmd.pipeline));

      command_buf.subpass(subpass_id);
      break;
    }

    default: break;
    }
  });

  command_buf
    .fenceSync(m_fence_id)  // Signal the fence when drawing is done
    .end();

  return command_buf;
}

void Ui::capture(Frame *frame)
{
  if(m_capture && frame != m_capture) m_capture->losingCapture();

  m_capture = frame;
}

void Ui::keyboard(Frame *frame)
{
  if(m_keyboard && frame != m_keyboard) m_keyboard->losingCapture();

  m_keyboard = frame;
}

}

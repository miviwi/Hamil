#include <gx/commandbuffer.h>
#include <gx/renderpass.h>
#include <gx/program.h>
#include <gx/vertex.h>

#include <cassert>

namespace gx {

CommandBuffer::CommandBuffer(size_t initial_alloc) :
  m_pool(nullptr),
  m_memory(nullptr),
  m_program(nullptr),
  m_renderpass(nullptr),
  m_last_draw(NonIndexedDraw)
{
  m_commands.reserve(initial_alloc);
}

CommandBuffer CommandBuffer::begin(size_t initial_alloc)
{
  return CommandBuffer(initial_alloc);
}

CommandBuffer& CommandBuffer::renderpass(ResourceId pass)
{
  return appendCommand(OpBeginRenderPass, pass);
}

CommandBuffer& CommandBuffer::subpass(uint id)
{
  if(id & ~OpDataMask) throw SubpassIdTooLargeError();

  return appendCommand(OpBeginSubpass, id);
}

CommandBuffer& CommandBuffer::program(ResourceId prog)
{
  checkResourceId(prog);

  return appendCommand(OpUseProgram, prog);
}

CommandBuffer& CommandBuffer::draw(Primitive p, ResourceId vertex_array, size_t num_verts)
{
  return appendCommand(make_draw(p, vertex_array, num_verts));
}

CommandBuffer& CommandBuffer::drawIndexed(Primitive p, ResourceId indexed_vertex_array, size_t num_inds)
{
  return appendCommand(make_draw_indexed(p, indexed_vertex_array, num_inds));
}

CommandBuffer & CommandBuffer::drawBaseVertex(Primitive p,
  ResourceId indexed_vertex_array, size_t num, u32 base, u32 offset)
{
  return appendCommand(make_draw_base_vertex(p, indexed_vertex_array, num))
    .appendExtraData(base)
    .appendExtraData(offset);
}

CommandBuffer& CommandBuffer::bufferUpload(ResourceId buf, MemoryPool::Handle h, size_t sz)
{
  checkResourceId(buf);

  return appendCommand(make_buffer_upload(buf, h, sz));
}

CommandBuffer& CommandBuffer::uniformInt(uint location, int value)
{
  union {
    int i;
    u32 data;
  } extra;

  extra.i = value;

  return appendCommand(OpPushUniform, make_push_uniform(OpDataUniformInt, location))
    .appendExtraData(extra.data);
}

CommandBuffer& CommandBuffer::uniformFloat(uint location, float value)
{
  union {
    float f;
    u32 data;
  } extra;

  extra.f = value;

  return appendCommand(OpPushUniform, make_push_uniform(OpDataUniformFloat, location))
    .appendExtraData(extra.data);
}

CommandBuffer& CommandBuffer::uniformSampler(uint location, uint sampler)
{
  return appendCommand(OpPushUniform, make_push_uniform(OpDataUniformSampler, location))
    .appendExtraData((u32)sampler);
}

CommandBuffer& CommandBuffer::uniformVector4(uint location, MemoryPool::Handle h)
{
  return appendCommand(OpPushUniform, make_push_uniform(OpDataUniformVector4, location))
    .appendExtraData(h);
}

CommandBuffer& CommandBuffer::uniformMatrix4x4(uint location, MemoryPool::Handle h)
{
  return appendCommand(OpPushUniform, make_push_uniform(OpDataUniformMatrix4x4, location))
    .appendExtraData(h);
}

CommandBuffer& CommandBuffer::fenceSync(ResourceId fence)
{
  if(fence & ~OpDataFenceOpDataMask) throw ResourceIdTooLargeError();

  u32 data = (OpDataFenceSync<<OpDataFenceOpShift) | fence;
  return appendCommand(OpFence, data);
}

CommandBuffer& CommandBuffer::fenceWait(ResourceId fence)
{
  if(fence & ~OpDataFenceOpDataMask) throw ResourceIdTooLargeError();

  u32 data = (OpDataFenceWait<<OpDataFenceOpShift) | fence;
  return appendCommand(OpFence, data);
}

CommandBuffer& CommandBuffer::end()
{
  return appendCommand(OpEnd);
}

CommandBuffer& CommandBuffer::bindResourcePool(ResourcePool *pool)
{
  m_pool = pool;

  return *this;
}

CommandBuffer& CommandBuffer::bindMemoryPool(MemoryPool *pool)
{
  m_memory = pool;

  return *this;
}

CommandBuffer& CommandBuffer::activeRenderPass(ResourceId renderpass)
{
  assertResourcePool();

  m_renderpass = &m_pool->get<RenderPass>(renderpass);

  return *this;
}

CommandBuffer& CommandBuffer::execute()
{
  if(m_commands.empty()) return *this;

  assert(m_commands.back() >> OpShift == OpEnd
    && "Attempted to execute() a CommandBuffer without a previous call to end() on it!\n"
    "(Or commands were added after the end() call)");
  assertResourcePool();

  u32 *pc = m_commands.data();
  while(auto next_pc = dispatch(pc)) {
    pc =  next_pc;
  }

  return *this;
}

CommandBuffer& CommandBuffer::reset()
{
  m_commands.clear();
  m_program = nullptr;

  return *this;
}

CommandBuffer& CommandBuffer::appendCommand(Command opcode, u32 data)
{
  assert((data & ~OpDataMask) == 0 && "OpData has overflown into the opcode!");
  assert(opcode < NumCommands && "The opcode is invalid!");

  u32 command = (opcode << OpShift) | data;
  m_commands.push_back(command);

  return *this;
}

CommandBuffer& CommandBuffer::appendExtraData(u32 data)
{
  m_commands.push_back(data);

  return *this;
}

CommandBuffer& CommandBuffer::appendCommand(CommandWithExtra c)
{
  return appendExtraData(c.command)
    .appendExtraData(c.extra);
}

CommandBuffer::u32 *CommandBuffer::dispatch(u32 *op)
{
  auto fetch_extra = [&]() -> CommandWithExtra
  {
    CommandWithExtra op_extra;

    op_extra.command = *op;
    op++;
    op_extra.extra = *op;

    return op_extra;
  };

  u32 command = *op;

  u32 opcode = op_opcode(command);
  u32 data   = op_data(command);

  switch(opcode) {
  case Nop: break;

  case OpBeginRenderPass:
    m_renderpass = &m_pool->get<RenderPass>(data)
      .begin(*m_pool);
    break;

  case OpBeginSubpass:
    assertRenderPass();

    m_renderpass->beginSubpass(*m_pool, data);
    break;

  case OpUseProgram:
    m_program = &m_pool->get<Program>(data)
      .use();
    break;

  case OpDraw:
  case OpDrawIndexed:
    assertProgram();

    drawCommand(fetch_extra());
    break;

  case OpDrawBaseVertex: {
    assertProgram();

    auto extra = fetch_extra();

    op++;
    u32 base = *op;
    op++;
    u32 offset = *op;

    drawCommand(extra, base, offset);
    break;
  }

  case OpBufferUpload:
    assertMemoryPool();

    uploadCommand(fetch_extra());
    break;

  case OpPushUniform:
    assertProgram();

    pushUniformCommand(fetch_extra());
    break;

  case OpFence:
    fenceCommand(data);
    break;

  case OpEnd:
    endIndexedArray();
    return nullptr;

  default: assert(0); // unreachable
  }

  return op + 1;
}

void CommandBuffer::drawCommand(CommandWithExtra op, u32 base, u32 offset)
{
  auto primitive = draw_primitive(op);
  auto array     = draw_array(op);
  auto num       = draw_num(op);

  if(m_last_draw != array) endIndexedArray();

  switch(op_opcode(op.command)) {
  case OpDraw:
    m_program->draw(primitive, m_pool->get<VertexArray>(array), offset, num);
    return;

  case OpDrawIndexed:
    m_program->draw(primitive, m_pool->get<IndexedVertexArray>(array), offset, num);
    m_last_draw = array;
    break;

  case OpDrawBaseVertex:
    m_program->drawBaseVertex(primitive, m_pool->get<IndexedVertexArray>(array), base, offset, num);
    m_last_draw = array;
    break;

  default: assert(0); // unreachable
  }
}

void CommandBuffer::uploadCommand(CommandWithExtra op)
{
  auto buffer = xfer_buffer(op);
  auto handle = xfer_handle(op);
  auto size   = xfer_size(op);

  auto ptr = m_memory->ptr(handle);

  m_pool->getBuffer(buffer).get().upload(ptr, 0, sizeof(byte), size);
}

void CommandBuffer::pushUniformCommand(CommandWithExtra op)
{
  auto type     = (op.command >> OpDataUniformTypeShift) & OpDataUniformTypeMask;
  auto location = op.command & OpDataUniformLocationMask;

  union {
    u32 data;

    int i;
    float f;
    MemoryPool::Handle h;
  } extra;

  extra.data = op.extra;

  switch(type) {
  case OpDataUniformInt:     m_program->uniformInt(location, extra.i); break;
  case OpDataUniformFloat:   m_program->uniformFloat(location, extra.f); break;
  case OpDataUniformSampler: m_program->uniformSampler(location, extra.data); break;
  case OpDataUniformVector4: m_program->uniformVector4(location, memoryPoolRef<vec4>(extra.h)); break;
  case OpDataUniformMatrix4x4: m_program->uniformMatrix4x4(location, memoryPoolRef<mat4>(extra.h)); break;

  default: throw UniformTypeInvalidError();
  }
}

void CommandBuffer::fenceCommand(u32 data)
{
  u32 op = (data>>OpDataFenceOpShift) & OpDataFenceOpMask;

  u32 fence_id = data & OpDataFenceOpDataMask;
  auto& fence = m_pool->get<gx::Fence>(fence_id);

  switch(op) {
  case OpDataFenceSync: fence.sync(); break;
  case OpDataFenceWait: fence.wait(); break;

  default: throw FenceOpInvalidError();
  }
}

void CommandBuffer::endIndexedArray()
{
  if(m_last_draw == NonIndexedDraw) return;

  m_pool->get<IndexedVertexArray>(m_last_draw).end();
  m_last_draw = NonIndexedDraw;
}

void CommandBuffer::checkResourceId(ResourceId id)
{
  if(id & ~OpDataMask) throw ResourceIdTooLargeError();
}

void CommandBuffer::checkNumVerts(size_t num)
{
  if(num & ~OpExtraNumVertsMask) throw NumVertsTooLargeError();
}

void CommandBuffer::checkHandle(MemoryPool::Handle h)
{
  if((h >> MemoryPool::AllocAlignShift) & ~OpExtraHandleMask) throw HandleOutOfRangeError();
  if(h & MemoryPool::AllocAlignMask) throw HandleUnalignedError();
}

void CommandBuffer::checkXferSize(size_t sz)
{
  if(sz & ~OpExtraXferSizeMask) throw XferSizeTooLargeError();
}

void CommandBuffer::checkUniformLocation(uint location)
{
  if(location & ~OpDataUniformLocationMask) throw UniformLocationTooLargeError();
}

void CommandBuffer::assertResourcePool()
{
  assert(m_pool && "This method requires a bound ResourcePool!");
}

void CommandBuffer::assertProgram()
{
  assert(m_program && "Command requires a bound Program!");
}

void CommandBuffer::assertMemoryPool()
{
  assert(m_memory && "Command requires a bound MemoryPool!");
}

void CommandBuffer::assertRenderPass()
{
  assert(m_renderpass && "Command requires an active RenderPass!");
}

CommandBuffer::Command CommandBuffer::op_opcode(u32 op)
{
  auto opcode = op >> OpShift;
  return (Command)(opcode & OpMask);
}

CommandBuffer::u32 CommandBuffer::op_data(u32 op)
{
  return op & OpDataMask;
}

static constexpr Primitive p_primitive_lut[] ={
  Points,
  Lines, LineLoop, LineStrip,
  Triangles, TriangleFan, TriangleStrip,

  (Primitive)~0u, // Sentinel
};

Primitive CommandBuffer::draw_primitive(CommandWithExtra op)
{
  auto primitive = (op.extra >> OpExtraPrimitiveShift) & OpExtraPrimitiveMask;
  return p_primitive_lut[primitive];
}

CommandBuffer::ResourceId CommandBuffer::draw_array(CommandWithExtra op)
{
  return op_data(op.command);
}

size_t CommandBuffer::draw_num(CommandWithExtra op)
{
  return (size_t)(op.extra & OpExtraNumVertsMask);
}

CommandBuffer::ResourceId CommandBuffer::xfer_buffer(CommandWithExtra op)
{
  return op_data(op.command);
}

MemoryPool::Handle CommandBuffer::xfer_handle(CommandWithExtra op)
{
  return (op_data(op.extra) & OpExtraHandleMask) << MemoryPool::AllocAlignShift;
}

size_t CommandBuffer::xfer_size(CommandWithExtra op)
{
  return (size_t)((op_data(op.extra) >> OpExtraXferSizeShift) & OpExtraXferSizeMask);
}

CommandBuffer::CommandWithExtra CommandBuffer::make_draw(Primitive p,
  ResourceId array, size_t num_verts)
{
  checkResourceId(array);

  // See p_primitive_lut above
  u32 primitive = 0;
  switch(p) {
  case Points: primitive = 0; break;

  case Lines:     primitive = 1; break;
  case LineLoop:  primitive = 2; break;
  case LineStrip: primitive = 3; break;

  case Triangles:     primitive = 4; break;
  case TriangleFan:   primitive = 5; break;
  case TriangleStrip: primitive = 6; break;

  default: primitive = 7; break; // Assign to sentinel
  }

  checkNumVerts(num_verts);
  auto num = (u32)(num_verts & OpExtraNumVertsMask);

  checkResourceId(array);

  CommandWithExtra c;
  c.command = (OpDraw << OpShift) | array;
  c.extra = (primitive << OpExtraPrimitiveShift) | num;

  return c;
}

CommandBuffer::CommandWithExtra CommandBuffer::make_draw_indexed(Primitive p,
  ResourceId array, size_t num_inds)
{
  auto c = make_draw(p, array, num_inds);

  c.command &= ~(OpMask << OpShift);
  c.command |= OpDrawIndexed << OpShift;

  return c;
}

CommandBuffer::CommandWithExtra CommandBuffer::make_draw_base_vertex(Primitive p,
  ResourceId array, size_t num_inds)
{
  auto c = make_draw(p, array, num_inds);

  c.command &= ~(OpMask << OpShift);
  c.command |= OpDrawBaseVertex << OpShift;

  return c;
}

CommandBuffer::CommandWithExtra CommandBuffer::make_buffer_upload(ResourceId buf,
  MemoryPool::Handle h, size_t sz)
{
  checkHandle(h);
  checkXferSize(sz);

  auto handle = (u32)(h >> MemoryPool::AllocAlignShift);
  auto size   = (u32)(sz & OpExtraXferSizeMask);

  CommandWithExtra c;
  c.command = (OpBufferUpload << OpShift) | buf;
  c.extra = (size << OpExtraXferSizeShift) | handle;

  return c;
}

CommandBuffer::u32 CommandBuffer::make_push_uniform(u32 type, uint location)
{
  checkUniformLocation(location);

  return (type << OpDataUniformTypeShift) | location;
}

}
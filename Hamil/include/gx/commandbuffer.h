#pragma once

#include <gx/gx.h>
#include <gx/resourcepool.h>
#include <gx/memorypool.h>

#include <vector>
#include <utility>

namespace gx {

class RenderPass;
class Program;

class CommandBuffer {
public:
  using u32 = ::u32;
  using u64 = ::u64;
  using ResourceId = ResourcePool::Id;

  using Command = u32;

  enum : u32 {
    CommandBits = sizeof(Command)*CHAR_BIT,

    OpBits  = 4,
    OpShift = CommandBits - OpBits,
    OpMask  = (1<<OpBits) - 1,

    OpDataBits = CommandBits - OpBits,
    OpDataMask = (1<<OpShift) - 1,
  };

  enum : u64 {
    // RawOpExtraShift = 32,

    OpExtraPrimitiveBits  = 3,
    OpExtraPrimitiveShift = CommandBits - OpExtraPrimitiveBits,
    OpExtraPrimitiveMask  = (1<<OpExtraPrimitiveBits) - 1,

    OpExtraNumVertsBits = CommandBits - OpExtraPrimitiveBits,
    OpExtraNumVertsMask = (1<<OpExtraNumVertsBits) - 1,

    OpExtraXferSizeBits  = 16,
    OpExtraXferSizeShift = 16,
    OpExtraXferSizeMask  = (1<<OpExtraXferSizeBits) - 1,

    OpExtraHandleBits = 16,
    OpExtraHandleMask = (1<<OpExtraHandleBits) - 1,
  };

  enum : Command {
    Nop,

    // The RenderPasses ResourcePool::Id is encoded in the OpData
    OpBeginRenderPass,

    // The Programs ResourcePool::Id is encoded in the OpData
    OpUseProgram,

    // CommandWithExtra where:
    //   - OpData encodes the VertexArray ResourceId
    //   - Bits [31;29] of OpExtra encode the Primitive
    //   - Bits [28;0] of OpExtra encode the num
    OpDraw, OpDrawIndexed,

    // CommandWithExtra where:
    //   - OpData encodes the UniformBuffer ResourceId
    //   - Bits [31;16] of OpExtra encode the upload size (in bytes)
    //   - Bits [15;0] of OpExtra encode the
   //        MemoryPool::Handle >> MemoryPool::AllocAlignBits
    OpUploadUniforms,
    OpSetUniform,

    OpEnd,

    NumCommands
  };
  static_assert(NumCommands < (1<<OpBits), "Too many opcodes defined!");

  struct Error {
  };

  struct ResourceIdTooLargeError : public Error {
  };

  struct NumVertsTooLargeError : public Error {
  };

  struct HandleOutOfRangeError : public Error {
  };

  struct XferSizeTooLargeError : public Error {
  };

  // Creates a new CommandBuffer with 'initial_alloc'
  //   preallocated commands
  static CommandBuffer begin(size_t initial_alloc = 64);

  CommandBuffer& renderPass(ResourceId pass);
  CommandBuffer& program(ResourceId prog);
  CommandBuffer& draw(Primitive p, ResourceId vertex_array, size_t num_verts);
  CommandBuffer& drawIndexed(Primitive p, ResourceId indexed_vertex_array, size_t num_inds);
  CommandBuffer& uploadUnifoms(ResourceId buf, MemoryPool::Handle h, size_t sz);

  // Must be called after the last recorded command!
  CommandBuffer& end();

  // The CommandBuffer must have a ResourcePool bound
  //   before execute() is called
  CommandBuffer& bindResourcePool(ResourcePool *pool);

  // The CommandBuffer must have a MemoryPool bound
  //   if upload commands will be used
  CommandBuffer& bindMemoryPool(MemoryPool *pool);

  CommandBuffer& execute();

protected:
  CommandBuffer(size_t initial_alloc);

private:
  enum {
    NonIndexedDraw = ~0u,
  };

  struct CommandWithExtra {
    union {
      struct {
        u32 command;
        u32 extra;
      };

      u64 raw;
    };
  };

  CommandBuffer& appendCommand(Command opcode, u32 data = 0);
  CommandBuffer& appendExtraData(u32 data);
  CommandBuffer& appendCommand(CommandWithExtra c);

  // Returns a pointer to the next Command or
  //   nullptr when the OpEnd Command is reached
  u32 *dispatch(u32 *op);
  void drawCommand(CommandWithExtra op);
  void uploadCommand(CommandWithExtra op);

  // Calls IndexedVertexArray::end() if
  //   the last draw command was drawIndexed()
  void endIndexedArray();

  static void checkResourceId(ResourceId id);
  static void checkNumVerts(size_t num);
  static void checkHandleRange(MemoryPool::Handle h);
  static void checkXferSize(size_t sz);

  void assertProgram();

  static Command op_opcode(u32 op);
  static u32 op_data(u32 op);

  static Primitive draw_primitive(CommandWithExtra op);
  static ResourceId draw_array(CommandWithExtra op);
  static size_t draw_num(CommandWithExtra op);

  static ResourceId xfer_buffer(CommandWithExtra op);
  static MemoryPool::Handle xfer_handle(CommandWithExtra op);
  static size_t xfer_size(CommandWithExtra op);

  static CommandWithExtra make_draw(Primitive p, ResourceId array, size_t num_verts);
  static CommandWithExtra make_draw_indexed(Primitive p, ResourceId array, size_t num_inds);

  static CommandWithExtra make_upload_uniforms(ResourceId buf, MemoryPool::Handle h, size_t sz);

  std::vector<u32> m_commands;
  ResourcePool *m_pool;
  MemoryPool *m_memory;
  Program *m_program;

  // Stores the last-used IndexedVertexArray or
  //   NonIndexedDraw otherwise
  u32 m_last_draw;

};

}
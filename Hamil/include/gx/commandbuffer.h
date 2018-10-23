#pragma once

#include <gx/gx.h>

#include <vector>

namespace gx {

class CommandBuffer {
public:
  using Command = ::u32;

  enum {
    CommandBits = sizeof(Command)*CHAR_BIT,

    OpBits  = 5,
    OpShift = CommandBits - OpBits,
    OpMask  = (1<<OpBits) - 1,
  };

  enum : Command {
    Nop,
    BeginPass,
    UseProgram,
    Draw, DrawIndexed,
    UploadUniforms,
    SetUniform,
  };

private:
  std::vector<Command> m_commands;
};

}
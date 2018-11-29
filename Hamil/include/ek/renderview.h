#pragma once

#include <ek/euklid.h>

namespace gx {
class CommandBuffer;
}

namespace ek {

class RenderView {
public:
  enum ViewType {
    Invalid,
    CameraView, LightView, ShadowView,
  };

  gx::CommandBuffer generateCommands();

private:
  ViewType m_type;
};

}
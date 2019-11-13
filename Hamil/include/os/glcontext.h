#pragma once

#include <os/os.h>

#include <memory>

namespace gx {
// Forward declaration
class GLContext;
}

namespace os {

std::unique_ptr<gx::GLContext> create_glcontext();

}

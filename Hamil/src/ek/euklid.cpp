#include <ek/euklid.h>

#include <ek/renderer.h>

#include <memory>

namespace ek {

std::unique_ptr<Renderer> p_renderer;

void init()
{
  p_renderer.reset(new Renderer());
}

void finalize()
{
  p_renderer.reset();
}

Renderer& renderer()
{
  return *p_renderer;
}

}
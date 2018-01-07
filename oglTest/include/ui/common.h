#pragma once

#include <common.h>

#include "vmath.h"

#include <map>
#include <utility>
#include <functional>

namespace ui {

struct Geometry {
  float x, y, w, h;

  Geometry(float x_, float y_, float w_, float h_) :
    x(x_), y(y_), w(w_), h(h_)
  { }
  Geometry(vec2 pos, float w_, float h_) :
    x(pos.x), y(pos.y), w(w_), h(h_)
  { }

  Geometry clip(const Geometry& g) const;
  Geometry contract(float factor) const;

  bool intersect(vec2 p) const;
  bool intersect(ivec2 p) const;

  vec2 center() const;
};

struct Color : public Vector4<byte> {
  Color(byte r_, byte g_, byte b_, byte a_) :
    Vector4<byte>(r_, g_, b_, a_)
  { }
  Color() :
    Vector4<byte>(0, 0, 0, 0)
  { }

  Color darken(unsigned factor) const;
  Color lighten(unsigned factor) const;
  Color luminance() const;
};

static Color transparent() { return Color{ 0, 0, 0, 0 }; }
static Color black() { return Color{ 0, 0, 0, 255 }; }
static Color white() { return Color{ 255, 255, 255, 255 }; }

using Position = Vector2<i16>;

template <typename... Args>
class Signal {
public:
  using Id = unsigned;
  using Slot = std::function<void(Args...)>;

  Signal() :
    m_current(0)
  { }
  Signal(const Signal& other) :
    m_current(0)
  { }

  Signal& operator=(const Signal& other) = delete;

  Id connect(const Slot& slot)
  {
    Id id = m_current++;

    m_slots.insert({ id, slot });
    return id;
  }

  template <typename C>
  Id connect(C *c, void (C::*fn)(Args...))
  {
    return connect([=](Args&&... args) -> void {
      (c->*fn)(std::forward<Args>(args)...);
    });
  }

  template <typename C>
  Id connect(C *c, void (C::*fn)(Args...) const)
  {
    return connect([=](Args&&... args) -> void {
      (c->*fn)(std::forward<Args>(args)...);
    });
  }

  void disconnect(Id id)
  {
    m_slots.erase(id);
  }

  void emit(Args... args)
  {
    for(auto& slot : m_slots) slot.second(std::forward<Args>(args)...);
  }

private:
  Id m_current;
  std::map<Id, Slot> m_slots;
};

}

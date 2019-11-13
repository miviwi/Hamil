#pragma once

#include <ui/uicommon.h>
#include <ui/ui.h>
#include <ui/frame.h>

#include <os/input.h>
#include <math/geometry.h>

#include <cfloat>

namespace ui {

class SliderFrame : public Frame {
public:
  using OnChange = Signal<SliderFrame *>;

  using Frame::Frame;
  virtual ~SliderFrame();

  virtual bool input(CursorDriver& cursor, const InputPtr& input);

  virtual void losingCapture();

  SliderFrame& range(double min, double max);
  SliderFrame& step(double step);
  SliderFrame& onChange(OnChange::Slot on_change);

  SliderFrame& value(double value);

  // Sets the value in the normalized range [min, max] -> [0, 1]
  SliderFrame& valuef(double fract);

  double value() const;
  int ivalue() const;

  OnChange& change();

protected:
  enum State {
    Default, Hover, Pressed,
  };

  constexpr static float Margin = 0.05f;
  constexpr static float InnerMargin = 0.06f;

  virtual double step() const = 0;

  virtual vec2 headPos() const = 0;

  double valueDelta() const;
  double valueFactor() const;
  double stepQuantize(double fine_value) const;
  double clampedValue(double value) const;

  virtual double clickedValue(vec2 pos) const = 0;

  State m_state = Default;

  double m_min  = 0;
  double m_max  = 100;
  double m_step = FLT_EPSILON;

  double m_value = 0;
  
  OnChange m_on_change;
};

class HSliderFrame : public SliderFrame {
public:
  using SliderFrame::SliderFrame;

  virtual void paint(VertexPainter& painter, Geometry parent);

  virtual vec2 sizeHint() const;

protected:
  virtual double step() const;
  virtual vec2 headPos() const;

  virtual double clickedValue(vec2 pos) const;

private:
  float width() const;
  float innerWidth() const;
};



}

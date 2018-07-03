#pragma once

#include <common.h>

#include <ui/uicommon.h>
#include <win32/time.h>

#include <cassert>
#include <initializer_list>
#include <utility>
#include <vector>
#include <numeric>

namespace ui {

enum AnimationEasing {
  EaseNone,
  EaseLinear,
  EaseQuadraticIn,
  EaseQuadraticOut,
  EaseQuadraticInOut,
  EaseCubicIn,
  EaseCubicOut,
  EaseCubicInOut,
  EaseQuinticIn,
  EaseQuinticOut,
  EaseQuinticInOut,
  EaseBack,
  EaseElastic,
  EaseBounce,
};

enum AnimationRepeat {
  RepeatOnce,
  RepeatLoop,
};

struct AnimationFrame {
  size_t first;
  float factor;
};

class IAnimationChannel {
public:
  size_t numKeyframes() const;
  float totalDuration() const;

protected:
  IAnimationChannel();

  static float ease(AnimationEasing easing, float factor);

  // time must be normalized (in the range <0; 1>)
  size_t findKeyframe(float time) const;
  // time must be passed directly from LoopTimer::elapsedLoopsf()
  AnimationFrame getFrame(float time) const;

  float m_total_duration;

  AnimationEasing m_easing;
  AnimationRepeat m_repeat;
  std::vector<float> m_timepoints;
};

template <typename T>
struct AnimationKeyframe {
  T value;
  float duration;
};

template <typename T>
static AnimationKeyframe<T> keyframe(T value, float duration)
{
  return AnimationKeyframe<T>{ value, duration };
}

template <typename T>
class AnimationChannel : public IAnimationChannel { 
public:
  using KeyframeInitList = std::initializer_list<AnimationKeyframe<T>>;

  AnimationChannel(KeyframeInitList keyframes,
                   AnimationEasing easing = EaseNone,
                   AnimationRepeat repeat = RepeatOnce)
  {
    m_easing = easing;
    m_repeat = repeat;

    m_total_duration = std::accumulate(keyframes.begin(), keyframes.end(), 0.0f,
                                   [](const auto& a, const auto& b) {
      return a + b.duration;
    });
    float current_time = 0.0f;

    m_timepoints.reserve(keyframes.size()+1);
    m_keyframes.reserve(keyframes.size()+1);

    for(auto& k : keyframes) {
      m_timepoints.push_back(current_time);
      m_keyframes.push_back(k.value);

      current_time += k.duration / totalDuration();
    }

    m_timepoints.push_back(1.0f);
    switch(repeat) {
    case RepeatOnce: m_keyframes.push_back(m_keyframes.back()); break;
    case RepeatLoop: m_keyframes.push_back(m_keyframes.front()); break;
    }
  }

private:
  friend class Animation;

  T value(float time)
  {
    auto frame = getFrame(time);
    auto factor = ease(m_easing, frame.factor);

    return lerp(m_keyframes[frame.first], m_keyframes[frame.first+1], factor);
  }

  std::vector<T> m_keyframes;
};

template <typename T>
IAnimationChannel *make_animation_channel(std::initializer_list<AnimationKeyframe<T>> keyframes,
                   AnimationEasing easing = EaseNone,
                   AnimationRepeat repeat = RepeatOnce)
{
  return new AnimationChannel<T>(keyframes, easing, repeat);
}

class Animation {
public:
  using ChannelList = std::initializer_list<IAnimationChannel *>;

  enum {
    MaxAnimationChannels = 8,
  };

  Animation();
  Animation(ChannelList channels);
  ~Animation();

  void init(ChannelList channels);

  void start();
  void stop();

  bool done();

  template <typename T>
  T channel(unsigned id)
  {
    auto chan = (AnimationChannel<T> *)m_channels[id];
    assert(chan && "AnimationChannel out of range!");

    return chan->value(m_timer.elapsedLoopsf());
  }

private:
  IAnimationChannel *m_channels[MaxAnimationChannels];

  win32::LoopTimer m_timer;
};

}
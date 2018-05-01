#include <ui/animation.h>
#include <math/compare.h>

#include <cmath>
#include <algorithm>

#include <Windows.h>

namespace ui {

Animation::Animation() :
  Animation({})
{
}

Animation::Animation(ChannelList channels)
{
  init(channels);
}

Animation::~Animation()
{
  for(auto chan : m_channels) {
    if(chan) delete chan;
  }
}

void Animation::init(ChannelList channels)
{
  if(!channels.size()) return;

  assert(channels.size() < MaxAnimationChannels && "MaxAnimationChannels exceeded!");

  float duration = channels.begin()[0]->totalDuration();
  for(auto chan : channels) {
    if(!chan) continue;

    assert(math::equal(chan->totalDuration(), duration) && "animation channels have varying length!");
  }

  std::fill(m_channels, m_channels+MaxAnimationChannels, nullptr);
  std::copy(channels.begin(), channels.end(), m_channels);

  m_timer.durationSeconds(m_channels[0]->totalDuration());
}

void Animation::start()
{
  m_timer.reset();
}

bool Animation::done()
{
  return m_timer.loops();
}

IAnimationChannel::IAnimationChannel()
{
}

size_t IAnimationChannel::findKeyframe(float time) const
{
  auto num_keyframes = m_timepoints.size();

  size_t first = 0;
  while(time > m_timepoints[first+1]) first++;
  
  return first;
} 

size_t IAnimationChannel::numKeyframes() const
{
  return m_timepoints.size();
}

float IAnimationChannel::totalDuration() const
{
  return m_total_duration;
}

AnimationFrame IAnimationChannel::getFrame(float time) const
{
  float loops;
  time = modf(time, &loops);

  if(m_repeat == RepeatOnce && loops >= 1.0f) time = 1.0f;

  auto keyframe = findKeyframe(time);

  float timepoints[] = {
    m_timepoints[keyframe],
    m_timepoints[keyframe+1]
  };
  float duration = timepoints[1] - timepoints[0];

  float factor = (time - timepoints[0]) / duration;

  return {
    keyframe,
    factor
  };
}

static float ease_bounce(float f)
{
  float ff = f*f;

  if(f < 4.0f/11.0f) {
    return (121.0f * ff)/16.0f;
  } else if(f < 8.0f/11.0f) {
    return (363.0f/40.0f * ff) - (99.0f/10.0f * f) + 17.0f/5.0;
  } else if(f < 9.0f/10.0f) {
    return (4356.0f/361.0f * ff) - (35442.0f/1805.0f * f) + 16061.0f/1805.0f;
  } 

  return (54.0f/5.0f * ff) - (513.0f/25.0f * f) + 268.0f/25.0f;
}

float IAnimationChannel::ease(AnimationEasing easing, float f)
{
  auto ff = f*f;
  auto fff = ff*f;
  auto fffff = fff*ff;
  auto q = f - 1.0f;
  auto r = 2.0f*f - 2.0f;

  switch(easing) {
  case EaseNone:           return 0.0f;
  case EaseLinear:         return f;
  case EaseQuadraticIn:    return ff;
  case EaseQuadraticOut:   return -(f * (f - 2.0f));
  case EaseQuadraticInOut: return f < 0.5f ? ff : -2.0f*ff + 4*f - 1;
  case EaseCubicIn:        return fff;
  case EaseCubicOut:       return q*q*q + 1;
  case EaseCubicInOut:     return f < 0.5f ? 4*fff : 0.5f*r*r*r + 1;
  case EaseQuinticIn:      return fffff;
  case EaseQuinticOut:     return q*q*q*q*q + 1.0f;
  case EaseQuinticInOut:   return f < 0.5f ? 16.0f*f*f*f*f*f : 0.5f*r*r*r*r*r + 1.0f;
  case EaseBack:           return fff - f*sin(f * PIf);
  case EaseElastic:        return sin(-13.0f * PIf*2.0f * (f + 1.0f)) * pow(2, -10.0f*f) + 1.0f;
  case EaseBounce:         return ease_bounce(f);
  }

  return NAN;
}

}
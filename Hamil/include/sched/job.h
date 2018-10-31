#pragma once

#include <sched/scheduler.h>

#include <win32/conditionvar.h>
#include <win32/time.h>

#include <atomic>
#include <tuple>
#include <functional>
#include <utility>

namespace sched {

class IJob {
public:
  IJob() = default;
  IJob(const IJob& other) = delete;

  virtual void perform() = 0;

  win32::ConditionVariable& condition();
  bool done();

  double dbg_ElapsedTime() const;

protected:
  void started();
  void finished();

private:
  std::atomic<bool> m_done;
  win32::ConditionVariable m_cv;

#if !defined(NDEBUG)
  win32::DeltaTimer m_timer;
  double m_dt;
#endif
};

struct Unit {
};

template <typename Ret, typename... Args>
class Job : public IJob {
public:
  using Params = std::tuple<Args...>;
  using Fn     = std::function<Ret(Args...)>;

  Job(Fn fn, Params params) :
    m_fn(fn), m_params(params)
  { }

  virtual void perform()
  {
    started();
    m_result = std::move(std::apply(m_fn, m_params));
    finished();
  }

  IJob *params(Args... args)
  {
    m_params = std::make_tuple(std::forward<Args>(args)...);

    return this;
  }

  Ret& result() { return m_result; }

private:
  Fn m_fn;

  Params m_params;
  Ret m_result;
};

template <typename Ret, typename... Args>
Job<Ret, Args...> create_job(typename Job<Ret, Args...>::Fn fn, Args... args)
{
  return Job<Ret, Args...>(fn, std::make_tuple(std::forward<Args>(args)...));
}

}
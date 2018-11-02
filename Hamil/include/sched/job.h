#pragma once

#include <sched/scheduler.h>

#include <util/lambdatraits.h>
#include <win32/conditionvar.h>
#include <win32/time.h>

#include <cassert>
#include <atomic>
#include <memory>
#include <tuple>
#include <functional>
#include <utility>
#include <optional>

namespace sched {

class WorkerPool;

class IJob {
public:
  IJob();
  IJob(const IJob& other) = delete;
  IJob(IJob&& other);

  // Can be called at any time to check if the Job has
  //   completed (after a call to WorkerPool::waitJob()
  //   the return value is guaranteed to be == true)
  bool done();

  // Returns the amount of time (in seconds) the Job took
  //   - Always returns 'INFINITY' in non-Debug builds
  double dbg_ElapsedTime() const;

protected:
  // Signaled after a job completes (finished() is called)
  win32::ConditionVariable& condition();

  void started();
  void finished();

  // Called by WorkerPool::scheduleJob() after
  //   a given job is added to the queue
  void scheduled();

  virtual void perform() = 0;

private:
  friend WorkerPool;

  std::atomic<bool> m_done;
  win32::ConditionVariable m_cv;

#if !defined(NDEBUG)
  win32::DeltaTimer m_timer;
  double m_dt;
#endif
};

template <typename Ret, typename... Args>
class Job : public IJob {
public:
  using Params = std::tuple<Args...>;
  using Fn     = std::function<Ret(Args...)>;

  Job(Fn fn, Params params) :
    m_fn(fn), m_params(params),
    m_result(std::nullopt)
  { }

  // Intended to be used with ex. WorkerPool::scheduleJob()
  //    pool.scheduleJob(job.withParams("some parameter", true, 3.14f))
  IJob *withParams(Args... args)
  {
    m_params = std::make_tuple(std::forward<Args>(args)...);

    return this;
  }

  // done() == true MUST be checked before calling this method
  Ret& result()
  {
    assert(done() && "Attempted to get result() of an in-flight Job!");

    return *m_result;
  }

protected:
  virtual void perform()
  {
    started();
    m_result = std::move(std::apply(m_fn, m_params));
    finished();
  }

private:
  Fn m_fn;

  Params m_params;
  std::optional<Ret> m_result;
};

// Infers the Jobs Result type from the passed Callable's return type
//   - Return 'Unit' <util/unit.h> when a void return type is desired
template <typename Fn, typename... Args>
Job<typename util::LambdaTraits<Fn>::RetType, Args...> create_job(Fn fn, Args... args)
{
  using FnTraits = util::LambdaTraits<Fn>;

  return Job<FnTraits::RetType, Args...>(fn, std::make_tuple(std::forward<Args>(args)...));
}

}
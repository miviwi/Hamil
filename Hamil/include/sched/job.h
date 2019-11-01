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
  virtual ~IJob() = default;

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

  Job(Fn&& fn, Params&& params) :
    m_fn(std::move(fn)), m_params(std::move(params)),
    m_result(std::nullopt)
  { }

  Job(Job&& other) :
    IJob(other),
    m_fn(std::move(other.m_fn)),
    m_params(std::move(other.m_params)), m_result(std::move(other.m_result))
  { }

  // Intended to be used with ex. WorkerPool::scheduleJob()
  //    pool.scheduleJob(job.withParams("some parameter", true, 3.14f))
  IJob *withParams(Args... args) &
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

namespace detail {

template <typename Ret, typename Arguments>
struct CreateJobHelperImpl;

template <typename Ret, typename... Args>
struct CreateJobHelperImpl<Ret, std::tuple<Args...>> {
  using JobType = Job<Ret, Args...>;

  static_assert(!std::is_same_v<Ret, void>, "Cannot use 'void' as Job return type (use 'Unit' instead)");

  template <typename Fn>
  static JobType create(Fn fn)
  {
    static_assert((std::is_default_constructible_v<Args> && ...),
      "create_job(Fn, ...Args) must be used when not all Job parameters are default constructible");

    return JobType(fn, std::make_tuple(Args()...));
  }

  template <typename Fn>
  static JobType create_with_args(Fn fn, Args... args)
  {
    return JobType(fn, std::make_tuple(std::forward<Args>(args)...));
  }
};

template <typename Fn>
struct CreateJobHelper : public CreateJobHelperImpl<
  typename util::LambdaTraits<Fn>::RetType,
  typename util::LambdaTraits<Fn>::Arguments> {

};

}

// Infers the Jobs Result type from the passed Callable's return type
//   - Return 'Unit' <util/unit.h> when a void return type is desired
//   - ...args become the parameters for the Job
template <typename Fn, typename... Args>
auto create_job(Fn fn, Args... args) ->
  typename detail::CreateJobHelper<Fn>::JobType
{
  using Helper = detail::CreateJobHelper<Fn>;

  return Helper::create_with_args(fn, std::forward<Args>(args)...);
}

// Same as create_job(Fn, ...Args), except the Job's parameters are default-initialized
//   - All the parameters must be default-constructible to use this overload
template <typename Fn>
auto create_job(Fn fn) ->
  typename detail::CreateJobHelper<Fn>::JobType
{
  using Helper = detail::CreateJobHelper<Fn>;

  return Helper::create(fn);
}

}

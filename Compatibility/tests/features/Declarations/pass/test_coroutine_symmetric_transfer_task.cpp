// EXPECT_COMPILE_ARGS: -fcoroutines
// EXPECT_COMPILE_ONLY: 1

// GCC row pr99047.C.  final_suspend can return a local awaiter whose
// await_ready result is std::false_type and whose await_suspend returns the
// stored continuation handle; co_return may contain a co_await chain.

#include <coroutine>
#include <optional>
#include <type_traits>
#include <utility>

template <typename T>
struct task
{
  struct promise_type
  {
    std::suspend_always initial_suspend() { return {}; }

    auto final_suspend() noexcept
    {
      struct awaiter
      {
        std::false_type await_ready() noexcept { return {}; }
        std::coroutine_handle<> await_suspend(std::coroutine_handle<>) noexcept
        {
          return next;
        }
        void await_resume() noexcept {}
        std::coroutine_handle<> next;
      };
      return awaiter{next};
    }

    void unhandled_exception() noexcept {}
    auto get_return_object() { return task(this); }
    auto coro()
    {
      return std::coroutine_handle<promise_type>::from_promise(*this);
    }
    void return_value(T v) { result.emplace(std::move(v)); }

    std::coroutine_handle<> next;
    std::optional<T> result;
  };

  task(task&& source) : p(std::exchange(source.p, nullptr)) {}
  explicit task(promise_type* p) : p(p) {}
  ~task()
  {
    if (p)
      p->coro().destroy();
  }

  bool await_ready() noexcept { return p->coro().done(); }
  std::coroutine_handle<> await_suspend(std::coroutine_handle<> next) noexcept
  {
    p->next = next;
    return p->coro();
  }
  const T& await_resume() const& noexcept { return *p->result; }

  promise_type* p;
};

task<int> five()
{
  co_return 5;
}

task<int> six()
{
  co_return co_await five() + 1;
}

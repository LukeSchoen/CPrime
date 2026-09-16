// EXPECT_COMPILE_ARGS: -fcoroutines
// EXPECT_COMPILE_ONLY: 1

// GCC row pr94883-folly-2.C.  A coroutine_traits specialization can derive
// from the return type to pick up promise_type from it, and await_transform
// may return the result of a member call on the await operand.

#include <coroutine>

struct promise;

struct task
{
  using promise_type = promise;

  bool await_ready();
  std::coroutine_handle<> await_suspend(std::coroutine_handle<>);
  int await_resume();
};

template <typename... Args>
struct std::coroutine_traits<task, Args...> : task {};

struct inner_awaiter
{
  bool await_ready();
  std::coroutine_handle<> await_suspend(std::coroutine_handle<>);
  bool await_resume();
};

struct operand
{
  inner_awaiter make(int);
};

struct outer
{
  operand get();
};

template <typename T>
auto make_awaiter(int count, T value) -> decltype(value.make(count));

struct promise
{
  task get_return_object();
  std::suspend_never initial_suspend();
  std::suspend_always final_suspend() noexcept;
  void return_void();
  void unhandled_exception();

  template <typename T>
  auto await_transform(T value)
  {
    return make_awaiter(0, value);
  }
};

template <typename T>
task transform(T value)
{
  if (bool alive = co_await value.get())
    ;
  co_return;
}

void instantiate_transform()
{
  transform(outer{});
}

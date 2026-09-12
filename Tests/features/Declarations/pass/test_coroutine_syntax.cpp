// EXPECT_COMPILE_ARGS: -fcoroutines

#include <coroutine>

struct task
{
  struct promise_type
  {
    task get_return_object() { return {}; }
    std::suspend_never initial_suspend() { return {}; }
    std::suspend_never final_suspend() noexcept { return {}; }
    std::suspend_never yield_value(int) { return {}; }
    void return_value(int) {}
    void unhandled_exception() {}
  };
};

struct awaiter
{
  bool await_ready() const { return true; }
  void await_suspend(std::coroutine_handle<>) {}
  int await_resume() { return 1; }
};

task run()
{
  int value = co_await awaiter{};
  co_yield value;
  co_return value;
}

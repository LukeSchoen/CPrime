// EXPECT_COMPILE_ARGS: -fcoroutines

#include <coroutine>

// `co_yield` and `co_return` lower into promise calls, so the promise is the
// only place the coroutine body's control transfer is observable until the
// frame machinery lands.
static int yielded, returned;

struct task
{
  struct promise_type
  {
    task get_return_object() { return {}; }
    std::suspend_never initial_suspend() { return {}; }
    std::suspend_never final_suspend() noexcept { return {}; }
    std::suspend_never yield_value(int value) { yielded = value; return {}; }
    void return_value(int value) { returned = value; }
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

int main()
{
  run();
  return yielded == 1 && returned == 1 ? 0 : 1;
}

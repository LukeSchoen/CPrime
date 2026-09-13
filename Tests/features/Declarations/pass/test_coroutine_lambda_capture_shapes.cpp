// EXPECT_COMPILE_ARGS: -fcoroutines
// EXPECT_COMPILE_ONLY: 1

// GCC rows pr116327-preview-this.C, pr103328.C, pr96517.C and pr96251.C.
// A lambda body may be a coroutine, capture the enclosing object, name members
// through the capture, and use co_yield/co_return inside control flow.

#include <coroutine>

struct eager
{
  struct promise_type
  {
    promise_type(const auto&...) {}
    eager get_return_object() { return {}; }
    std::suspend_never initial_suspend() { return {}; }
    std::suspend_always final_suspend() noexcept { return {}; }
    void return_value(int) noexcept {}
    void unhandled_exception() {}
  };
};

void capturing_lambda()
{
  auto f = [captured = 0](auto) -> eager { co_return 2; };
  f(0);
}

struct member_task
{
  struct promise_type
  {
    member_task get_return_object() { return {}; }
    std::suspend_never initial_suspend() { return {}; }
    std::suspend_never final_suspend() noexcept { return {}; }
    void unhandled_exception() {}
  };

  bool await_ready() { return false; }
  void await_suspend(std::coroutine_handle<>) {}
  void await_resume() {}
};

template <typename Func>
void call(Func function) { function(); }

struct member_lambda
{
  member_task awaited();

  void run()
  {
    auto lambda = [this]() noexcept -> member_task { co_await awaited(); };
    (void)call<decltype(lambda)>;
  }
};

struct data
{
  constexpr int get() { return 5; }
};

struct captured_member
{
  data value;

  void run()
  {
    [this]() -> member_task
    {
      value.get();
      co_return;
    };
  }
};

struct generator
{
  struct promise_type
  {
    generator get_return_object() { return {}; }
    std::suspend_always initial_suspend() { return {}; }
    std::suspend_always yield_value(int) { return {}; }
    std::suspend_always final_suspend() noexcept { return {}; }
    void return_void() {}
    void unhandled_exception() {}
  };
};

void control_flow_lambdas()
{
  auto loop = [](auto max) -> generator
  {
    for (int i = 0; i < max; ++i)
      co_yield i;
  };
  loop(10);

  auto choose = [](auto&&) -> generator
  {
    switch (42)
    {
      case 42:
        co_return;
    }
  };
  choose(1);
}

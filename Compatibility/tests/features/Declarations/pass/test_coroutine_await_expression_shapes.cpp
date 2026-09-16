// EXPECT_COMPILE_ARGS: -fcoroutines
// EXPECT_COMPILE_ONLY: 1

// GCC rows pmf-121094.C, pr111728.C, pr112341.C, pr116502.C, pr116793-1.C
// and pr121643.C.  The co_await operand can be a pointer-to-member call, an
// immediately invoked lambda, a yielded value, a temporary with a nodiscard
// reference result, or a structured-binding initializer; the awaiter's
// await_suspend may have default arguments.

#include <coroutine>
#include <tuple>

struct result
{
  struct promise_type
  {
    result get_return_object() { return {}; }
    std::suspend_never initial_suspend() noexcept { return {}; }
    std::suspend_always final_suspend() noexcept { return {}; }
    void return_void() {}
    void unhandled_exception() {}
  };

  bool await_ready() { return false; }
  void await_suspend(std::coroutine_handle<>) {}
  void await_resume() {}
};

struct caller
{
  result run(result (caller::*fn)())
  {
    co_await (this->*fn)();
    co_return;
  }
};

struct handle_promise;
struct handle_result : std::coroutine_handle<handle_promise>
{
  using promise_type = handle_promise;
  bool await_ready() { return false; }
  void await_suspend(coroutine_handle handle) {}
  int await_resume() { return {}; }
};

struct handle_promise
{
  handle_result get_return_object()
  {
    return {handle_result::from_promise(*this)};
  }
  std::suspend_always initial_suspend() noexcept { return {}; }
  std::suspend_always final_suspend() noexcept { return {}; }
  void return_void() {}
  void unhandled_exception() {}
};

handle_result nested_lambda()
{
  int static_buffer[10];
  co_await [](auto) -> handle_result
  {
    if (sizeof(static_buffer))
      co_return;
  }(0);
}

struct field { int j; };

struct field_awaiter
{
  bool await_ready();
  bool await_suspend(std::coroutine_handle<>);
  field await_resume();
};

struct field_task
{
  struct promise_type
  {
    std::suspend_always initial_suspend();
    std::suspend_always final_suspend() noexcept;
    void unhandled_exception();
    field_task get_return_object();
    void return_void();
    field_awaiter yield_value(auto) { return {}; }
  };
};

field_task field_from_await(auto)
{
  (co_await field_awaiter()).j;
}

field_task field_from_yield(auto)
{
  (co_yield 0).j;
}

void instantiate_field_tasks()
{
  field_from_await(0);
  field_from_yield(0);
}

struct reference_awaiter
{
  bool await_ready();
  void await_suspend(std::coroutine_handle<>);
  [[nodiscard]] int& await_resume();
};

result discard_reference()
{
  (void)co_await reference_awaiter{};
}

struct cleaning_task
{
  bool await_ready() const noexcept;
  template <typename Promise>
  bool await_suspend(std::coroutine_handle<Promise> parent) noexcept;
  std::tuple<int&> await_resume() noexcept;
};

struct binding_task
{
  struct promise_type
  {
    std::suspend_always initial_suspend() noexcept;
    std::suspend_always final_suspend() noexcept;
    void unhandled_exception() noexcept;
    binding_task get_return_object() noexcept;
  };
};

cleaning_task get_cleaning_task(int&&);

binding_task structured_binding()
{
  auto&& [value] = co_await get_cleaning_task(3);
}

struct default_argument_awaiter
{
  bool await_ready() noexcept { return true; }
  void await_suspend(std::coroutine_handle<>, int sloc = 1) noexcept {}
  void await_resume() noexcept {}
};

result default_argument()
{
  co_await default_argument_awaiter{};
}

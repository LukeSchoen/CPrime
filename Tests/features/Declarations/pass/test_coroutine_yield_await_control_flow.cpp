// EXPECT_COMPILE_ARGS: -fcoroutines
// EXPECT_COMPILE_ONLY: 1

// GCC rows pr94886-folly-3.C, pr98480.C, pr95345.C, pr99575.C, pr95050.C and
// pr113457-1.C.  co_yield and co_await appear in range-for and for-condition
// statements, in fold expressions, under an await operator, initializing a
// non-copyable object, and mixed into a comma expression.

#include <array>
#include <coroutine>

struct generator
{
  struct promise_type
  {
    generator get_return_object();
    void return_void();
    void unhandled_exception();
    std::suspend_always initial_suspend();
    std::suspend_always final_suspend() noexcept;
    std::suspend_always yield_value(int) { return {}; }
  };
};

generator values()
{
  const std::array<int, 5> expected = {{0, 3, 1, 4, 2}};
  for (int value : expected)
    co_yield value;
}

struct future
{
  struct promise_type
  {
    void return_value(int) {}
    std::suspend_never initial_suspend() { return {}; }
    std::suspend_never final_suspend() noexcept { return {}; }
    void unhandled_exception() {}
    future get_return_object() { return {}; }
  };

  bool await_ready() { return true; }
  void await_suspend(std::coroutine_handle<>) {}
  int await_resume() { return 0; }
};

future loop_condition()
{
  for (int i = 0; i < co_await future{}; ++i)
    ;
  co_return 0;
}

struct self_promise
{
  using promise_type = self_promise;
  self_promise get_return_object() { return {}; }
  bool await_ready() noexcept { return false; }
  void await_suspend(std::coroutine_handle<>) noexcept {}
  void await_resume() noexcept {}
  self_promise initial_suspend() { return {}; }
  self_promise final_suspend() noexcept { return {}; }
  void return_void() {}
  void unhandled_exception() {}
};

template <int... Values>
self_promise fold_await()
{
  ((co_await [](int) { return std::suspend_never{}; }(Values)), ...);
  co_return;
}

void instantiate_fold_await()
{
  fold_await<1>();
}

struct move_task
{
  struct promise_type
  {
    move_task get_return_object() { return {}; }
    std::suspend_always initial_suspend() { return {}; }
    std::suspend_always final_suspend() noexcept { return {}; }
    void unhandled_exception() {}
    void return_void() {}
  };

  bool await_ready() const { return false; }
  void await_suspend(std::coroutine_handle<void>) {}
  void await_resume() {}
};

struct nonmove_task
{
  nonmove_task() = default;
  nonmove_task(const nonmove_task&) = delete;
  nonmove_task(nonmove_task&&) = delete;
  nonmove_task& operator=(const nonmove_task&) = delete;
  nonmove_task& operator=(nonmove_task&&) = delete;

  bool await_ready() const { return false; }
  void await_suspend(std::coroutine_handle<void>) {}
  void await_resume() {}
};

move_task await_pointer(nonmove_task* task)
{
  co_await* task;
}

struct noncopyable
{
  noncopyable() = default;
  noncopyable(const noncopyable&) = delete;
  ~noncopyable() {}
};

struct noncopyable_awaiter
{
  bool await_ready() const { return true; }
  void await_suspend(std::coroutine_handle<>) {}
  noncopyable await_resume() { return {}; }
};

struct construct_task
{
  struct promise_type
  {
    construct_task get_return_object() { return {}; }
    std::suspend_always initial_suspend() { return {}; }
    std::suspend_always final_suspend() noexcept { return {}; }
    void return_void() {}
    void unhandled_exception() {}
  };
};

construct_task construct_from_await()
{
  noncopyable value{co_await noncopyable_awaiter{}};
}

struct comma_task
{
  struct promise_type
  {
    std::suspend_never initial_suspend();
    std::suspend_never final_suspend() noexcept;
    void return_void();
    void unhandled_exception();
    comma_task get_return_object();
  };
};

struct derived_suspend_never : std::suspend_never {};

comma_task comma_await()
{
  co_await std::suspend_never{},
    []() -> comma_task { co_return; },
    co_await derived_suspend_never{};
}

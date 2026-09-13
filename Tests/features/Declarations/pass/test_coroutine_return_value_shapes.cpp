// EXPECT_COMPILE_ARGS: -fcoroutines
// EXPECT_COMPILE_ONLY: 1

// GCC rows pr112341-3.C, pr116880.C and pr95591.C.  co_return and co_yield
// select promise return_value/yield_value overloads and templates, including
// an await_transform that forwards an rvalue coroutine object.

#include <coroutine>

struct done {};

struct coroutine
{
  struct promise_type
  {
    std::suspend_always initial_suspend() noexcept;
    std::suspend_always final_suspend() noexcept;
    coroutine get_return_object();
    void return_value(done);
    void return_value(coroutine&&);
    void unhandled_exception();
    coroutine&& await_transform(coroutine&& value)
    {
      return static_cast<coroutine&&>(value);
    }
  };

  bool await_ready() { return false; }
  std::coroutine_handle<> await_suspend(std::coroutine_handle<promise_type>);
  void await_resume() {}
};

coroutine next();

coroutine forward()
{
  co_await next();
  co_return done{};
}

struct yield_once
{
  struct promise_type
  {
    std::suspend_always initial_suspend();
    std::suspend_always final_suspend() noexcept;
    yield_once get_return_object();
    void return_void();
    void unhandled_exception();
    std::suspend_never yield_value(int);
  };
};

struct yield_or_return
{
  struct promise_type
  {
    std::suspend_always initial_suspend();
    std::suspend_always final_suspend() noexcept;
    yield_or_return get_return_object();
    void return_value(auto);
    void unhandled_exception();
    std::suspend_never yield_value(auto);
  };
};

template <typename> yield_once return_void() { co_return; }
template <typename> yield_once yield_int() { co_yield 123; }
template <typename> yield_or_return return_value() { co_return 123; }
template <typename> yield_or_return yield_value() { co_yield 123; }

void instantiate_return_value_shapes()
{
  return_void<int>();
  return_void<bool>();
  yield_int<int>();
  yield_int<bool>();
  return_value<int>();
  return_value<bool>();
  yield_value<int>();
  yield_value<bool>();
}

struct generator
{
  struct promise_type
  {
    generator get_return_object();
    void return_void();
    void unhandled_exception();
    std::suspend_always initial_suspend();
    std::suspend_always final_suspend() noexcept;

    template <typename Arg>
    std::suspend_always yield_value(Arg&&) { return {}; }
  };
};

generator words()
{
  co_yield "foo";
}

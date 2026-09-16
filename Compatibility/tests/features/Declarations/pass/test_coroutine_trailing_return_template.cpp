// A coroutine function template that spells its result after `->` keeps that
// written result: the leading `auto` is only a placeholder, so the body is
// never probed for a deduced return type and the promise is resolved from the
// trailing type instead.

// EXPECT_COMPILE_ARGS: -fcoroutines
// EXPECT_COMPILE_ONLY: 1

#include <coroutine>

template <typename R>
struct elements_of
{
  R range;
};

struct task
{
  struct promise_type
  {
    std::suspend_always initial_suspend () const noexcept;
    std::suspend_never final_suspend () noexcept;
    template <typename R>
    std::suspend_always yield_value (elements_of<R> element) noexcept;
    void unhandled_exception () {}
    void return_void ();
    task get_return_object () noexcept;
  };
};

template <typename R>
auto concat (R &&range) -> task
{
  co_yield elements_of<R &&>{range};
}

template <typename T>
auto number (T value) -> int
{
  return (int) value;
}

int main ()
{
  int const numbers[] = {1, 2};

  concat (numbers);
  return number (3) == 3 ? 0 : 1;
}

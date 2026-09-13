// EXPECT_COMPILE_ARGS: -fcoroutines
// EXPECT_COMPILE_ONLY: 1

// GCC row pr94760-mismatched-traits-and-promise-prev.C.  A coroutine_traits
// specialization for a function object passes the object and its call
// arguments to the promise constructor; the return object need not be the
// promise's own type.

#include <coroutine>

struct fake {};

template <typename Result, typename CallOp, typename... Args>
struct std::coroutine_traits<Result, CallOp, Args...>
{
  struct promise_type
  {
    promise_type(CallOp, Args...) {}
    fake get_return_object() { return {}; }
    std::suspend_always initial_suspend() { return {}; }
    std::suspend_never final_suspend() noexcept { return {}; }
    void return_void() {}
    void unhandled_exception() {}
  };
};

struct functor
{
  fake operator()(int) { co_return; }
};

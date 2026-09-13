// EXPECT_COMPILE_ARGS: -fcoroutines
// EXPECT_COMPILE_ONLY: 1

// GCC row pr98118.C.  A coroutine whose by-value parameter has a non-trivial
// destructor needs the parameter moved into the frame, and quick-exit
// suspension both before and after the body must accept it.

#include <coroutine>

struct parameter
{
  ~parameter();
};

struct fire_and_forget
{
  struct promise_type
  {
    fire_and_forget get_return_object();
    std::suspend_never initial_suspend();
    std::suspend_never final_suspend() noexcept;
    void return_void();
    void unhandled_exception();
  };
};

fire_and_forget run(parameter value)
{
  co_return;
}

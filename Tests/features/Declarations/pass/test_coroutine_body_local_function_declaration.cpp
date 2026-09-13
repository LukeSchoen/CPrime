// EXPECT_COMPILE_ARGS: -fcoroutines
// EXPECT_COMPILE_ONLY: 1

// A function declaration is a statement, so a coroutine body may declare one
// before its co_return.  This is the shape retained GCC row
// coro-function-decl.C compiles.

#include <coroutine>

struct task
{
  struct promise_type
  {
    std::suspend_always initial_suspend() { return {}; }
    std::suspend_always final_suspend() noexcept { return {}; }
    void unhandled_exception() {}
    task get_return_object() noexcept { return {}; }
    void return_void() {}
  };
};

task declared_local()
{
  void bar();
  co_return;
}

// EXPECT_COMPILE_ARGS: -fcoroutines
// EXPECT_COMPILE_ONLY: 1

// GCC rows pr109682.C and pr95440.C.  Promise methods may be static and
// constexpr, and get_return_object_on_allocation_failure may have a different
// type from get_return_object.

#include <coroutine>

struct allocation_task
{
  allocation_task();
  allocation_task(int);

  struct promise_type
  {
    allocation_task get_return_object() { return {}; }
    static int get_return_object_on_allocation_failure() { return {}; }
    std::suspend_never initial_suspend() noexcept { return {}; }
    std::suspend_never final_suspend() noexcept { return {}; }
    void return_void() {}
    void unhandled_exception() {}
  };
};

allocation_task allocate()
{
  co_return;
}

struct constant_task
{
  struct promise_type
  {
    constexpr constant_task get_return_object() const { return {}; }
    static constexpr std::suspend_always initial_suspend() { return {}; }
    static constexpr std::suspend_never final_suspend() noexcept { return {}; }
    static constexpr void return_void() {}
    static constexpr void unhandled_exception() {}
  };
};

constant_task constant_await()
{
  co_await std::suspend_always{};
}

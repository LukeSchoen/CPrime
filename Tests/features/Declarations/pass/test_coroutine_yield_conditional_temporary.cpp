// EXPECT_COMPILE_ARGS: -fcoroutines
// EXPECT_COMPILE_ONLY: 1

// `yield_value` takes its argument by value, so a conditional expression that
// produces one of two prvalue temporaries has to be materialized for the
// promise without an internal compiler error.  This is the shape retained GCC
// row pr109283.C compiles.

#include <coroutine>

struct foo
{
    ~foo();
};

struct task
{
    struct promise_type
    {
        std::suspend_never initial_suspend();
        std::suspend_never final_suspend() noexcept;
        std::suspend_never yield_value(foo);
        void return_void();
        void unhandled_exception();
        task get_return_object();
    };
};

task source(int b)
{
    co_yield b ? foo{} : foo{};
}

// EXPECT_COMPILE_ARGS: -fcoroutines
// EXPECT_COMPILE_ONLY: 1

// The return type of a coroutine names its promise through `promise_type`, and
// the promise's `get_return_object()` result converts to that return type, so
// an alias plus a converting constructor is enough.

#include <coroutine>
#include <utility>

struct task
{
    struct promise_type
    {
        task get_return_object();
        void return_void();
        void unhandled_exception();
        std::suspend_always initial_suspend() noexcept;
        std::suspend_always final_suspend() noexcept;
    };
};

struct wrapper
{
    using promise_type = task::promise_type;
    wrapper(task &&);
};

wrapper aliased_promise()
{
    co_return;
}

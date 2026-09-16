// EXPECT_COMPILE_ARGS: -fcoroutines
// EXPECT_COMPILE_ONLY: 1

// A coroutine parameter is copied or moved into the frame before the body is
// entered, so a by-value move-only parameter has to be accepted.

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

struct move_only
{
    move_only();
    move_only(const move_only&) = delete;
    move_only(move_only&) = delete;
    move_only(move_only&&) = default;
};

task take_move_only(move_only value)
{
    co_return;
}

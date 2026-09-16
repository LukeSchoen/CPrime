// EXPECT_COMPILE_ARGS: -fcoroutines
// EXPECT_COMPILE_ONLY: 1

// The coroutine's own return object can have a destructor that is not
// noexcept; the promise type is still found in the class that declares it.

#include <coroutine>

struct Foo
{
    ~Foo() noexcept(false);

    struct promise_type
    {
        Foo get_return_object() { return {}; }
        std::suspend_never initial_suspend() { return {}; }
        void return_void() {}
        void unhandled_exception() {}
        std::suspend_always final_suspend() noexcept { return {}; }
    };
};

Foo run()
{
    co_return;
}

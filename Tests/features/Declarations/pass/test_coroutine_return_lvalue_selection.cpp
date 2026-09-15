// EXPECT_COMPILE_ARGS: -fcoroutines
// EXPECT_COMPILE_ONLY: 1
#include <coroutine>
struct Value {};
struct LvalueTask {
    struct promise_type {
        LvalueTask get_return_object() { return {}; }
        std::suspend_always initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        void return_value(Value&) {}
        void unhandled_exception() {}
    };
};
LvalueTask fallback() { Value local; co_return (local); }
struct RestrictedTask {
    struct promise_type {
        RestrictedTask get_return_object() { return {}; }
        std::suspend_always initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        void return_value(Value&) {}
        void return_value(Value&&) = delete;
        void unhandled_exception() {}
    };
};
RestrictedTask reference(Value& value) { co_return value; }
RestrictedTask persistent() { static Value value; co_return value; }

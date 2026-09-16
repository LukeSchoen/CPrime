#include <exception>
#include <stdlib.h>

static int moves;
static int fail_move;
struct Value {
    int number;
    Value() noexcept : number(23) {}
    Value(const Value&) = delete;
    Value(Value&& source) : number(source.number) {
        ++moves;
        if (fail_move) throw 11;
        source.number = 0;
    }
};
static void terminated() {
    _Exit(moves == 2 && std::uncaught_exceptions() == 1 ? 0 : 1);
}
int main() {
    struct Owner {
        Value value;
        Owner() = default;
        Owner(const Owner&) = delete;
        Owner(Owner&&) noexcept = default;
    };
    std::set_terminate(terminated);
    Owner source;
    static_assert(noexcept(Owner(static_cast<Owner&&>(source))), "move specification");
    Owner moved(static_cast<Owner&&>(source));
    if (moves != 1 || moved.value.number != 23 || source.value.number != 0) return 2;
    fail_move = 1;
    try { Owner failed(static_cast<Owner&&>(moved)); } catch (...) { return 3; }
    return 4;
}

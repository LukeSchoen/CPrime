#include <exception>
#include <stdlib.h>

static int copies;
static int fail_copy;
struct Value {
    int number;
    Value() noexcept : number(17) {}
    Value(const Value& source) : number(source.number) { ++copies; if (fail_copy) throw 7; }
};
struct Owner {
    Value value;
    Owner() = default;
    Owner(const Owner&) noexcept = default;
};
static void terminated() {
    _Exit(copies == 1 && std::uncaught_exceptions() == 1 ? 0 : 1);
}
int main() {
    std::set_terminate(terminated);
    Owner source;
    Owner successful(source);
    if (successful.value.number != 17 || copies != 1) return 4;
    copies = 0;
    fail_copy = 1;
    try { Owner copy(source); } catch (...) { return 2; }
    return 3;
}

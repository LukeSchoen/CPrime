#include <exception>
#include <stdlib.h>
static int assignments;
static int fail_assignment;
struct Value {
    int number;
    Value& operator=(Value&& other) {
        ++assignments;
        if (fail_assignment) throw 13;
        number = other.number;
        other.number = 0;
        return *this;
    }
};
struct Owner {
    Value value;
    Owner& operator=(Owner&&) noexcept = default;
};
static void terminated() {
    _Exit(assignments == 2 && std::uncaught_exceptions() == 1 ? 0 : 1);
}
int main() {
    std::set_terminate(terminated);
    Owner source = {{31}}, target = {{0}};
    if (&(target = static_cast<Owner&&>(source)) != &target
        || assignments != 1 || target.value.number != 31 || source.value.number != 0) return 2;
    fail_assignment = 1;
    try { source = static_cast<Owner&&>(target); } catch (...) { return 3; }
    return 4;
}

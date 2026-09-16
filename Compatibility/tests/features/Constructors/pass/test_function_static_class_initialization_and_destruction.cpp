// EXPECT_EXIT: 0
// EXPECT_COMPILE_ARGS: -Werror
#include <stdlib.h>
static int constructed, destroyed, attempts;
static unsigned long long order;
struct CheckExit {
    ~CheckExit() {
        if (constructed != 7 || destroyed != 7 || order != 7654321) abort();
    }
} check_exit;
struct Object {
    int value;
    Object() : value(4) { ++constructed; }
    explicit Object(int number) : value(number) {
        if (number == 7 && attempts++ == 0) throw 23;
        ++constructed;
    }
    Object(const Object&) = delete;
    ~Object() { ++destroyed; order = order * 10 + value; }
};
Object& during_global() { static Object value(1); return value; }
struct Global {
    Global() { during_global(); ++constructed; }
    ~Global() { ++destroyed; order = order * 10 + 2; }
} global;
Object& direct(int number) { static Object value(number); return value; }
Object& defaulted() { static Object value; return value; }
Object& braced() { static Object value{5}; return value; }
Object& copied() { static Object value = Object(6); return value; }
Object& retry() { static Object value(7); return value; }
int main() {
    if (constructed != 2 || destroyed) return 1;
    if (direct(3).value != 3 || direct(99).value != 3) return 2;
    if (defaulted().value != 4 || braced().value != 5) return 3;
    if (copied().value != 6 || &copied() != &copied()) return 4;
    try { retry(); return 5; } catch (int value) { if (value != 23) return 6; }
    if (constructed != 6 || destroyed) return 7;
    if (retry().value != 7 || retry().value != 7 || attempts != 2) return 8;
    return constructed != 7 || destroyed;
}

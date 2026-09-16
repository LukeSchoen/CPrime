// EXPECT_EXIT: 0
// EXPECT_COMPILE_ARGS: -Werror
#include <stdlib.h>
static int attempts, constructed, destroyed, order;
struct CheckExit {
    ~CheckExit() {
        if (constructed != 4 || destroyed != 4 || order != 1543) abort();
    }
} check_exit;
struct Object {
    int value;
    Object() : value(++attempts) {
        if (value == 2) throw 29;
        ++constructed;
    }
    ~Object() { ++destroyed; order = order * 10 + value; }
};
Object *objects() { static Object values[3]; return values; }
int main() {
    try { objects(); return 1; } catch (int value) { if (value != 29) return 2; }
    if (attempts != 2 || constructed != 1 || destroyed != 1) return 3;
    Object *value = objects();
    if (value[0].value != 3 || value[1].value != 4 || value[2].value != 5) return 4;
    if (objects() != value || attempts != 5) return 5;
    return constructed != 4 || destroyed != 1;
}

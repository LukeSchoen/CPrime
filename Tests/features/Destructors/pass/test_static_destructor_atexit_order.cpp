// EXPECT_STDOUT: 43218
#include <stdio.h>
#include <stdlib.h>
struct Object {
    int digit;
    explicit Object(int value) : digit(value) {}
    ~Object() { putchar('0' + digit); }
};
Object namespace_object = Object(8);
static void first_callback() { putchar('1'); }
static void third_callback() { putchar('3'); }
int main() {
    if (atexit(first_callback)) return 1;
    static Object first(2);
    if (atexit(third_callback)) return 2;
    static Object second(4);
    return 0;
}

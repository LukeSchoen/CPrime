// EXPECT_EXIT: 0
// EXPECT_COMPILE_ARGS: -Werror
#include <stdlib.h>
static int destroyed;
struct CheckExit { ~CheckExit() { if (destroyed != 1) abort(); } } check_exit;
struct Object { ~Object() { ++destroyed; } };
void initialize() { static Object value; }
int main() {
    initialize(); initialize();
    if (destroyed) return 1;
    exit(0);
}

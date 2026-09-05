// EXPECT_EXIT: 0
// EXPECT_COMPILE_ARGS: -Werror
#include <thread>
static int constructions;
struct Shared {
    int value;
    Shared() : value(0) {
        ++constructions;
        for (volatile int i = 0; i < 100000; ++i) {}
        value = 37;
    }
};
Shared& shared() { static Shared value; return value; }
static int first_result, second_result, third_result;
int main() {
    std::thread first([] { first_result = shared().value; });
    std::thread second([] { second_result = shared().value; });
    std::thread third([] { third_result = shared().value; });
    int result = shared().value;
    first.join(); second.join(); third.join();
    return constructions != 1 || first_result != 37 || second_result != 37
        || third_result != 37 || result != 37 || &shared() != &shared();
}

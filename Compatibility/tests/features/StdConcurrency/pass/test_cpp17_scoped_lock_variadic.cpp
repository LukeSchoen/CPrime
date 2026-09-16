// EXPECT_EXIT: 0
// EXPECT_COMPILE_ARGS: -std=c++17
// Coverage: scoped_lock accepts any number of mutexes, locks each of them,
// unlocks each exactly once, and keeps the template-free spelling
// `std::scoped_lock lock(m);` working.
#include <mutex>

static int locks = 0;
static int unlocks = 0;

struct Counting {
    void lock() { ++locks; }
    void unlock() { ++unlocks; }
};

int main() {
    Counting first;
    Counting second;
    std::mutex real;
    {
        std::scoped_lock one(real);
        (void)one;
    }
    {
        std::scoped_lock one(first);
        if (locks != 1 || unlocks != 0) return 1;
    }
    if (unlocks != 1) return 2;
    {
        std::scoped_lock two(first, second);
        if (locks != 3) return 3;
    }
    if (locks != 3 || unlocks != 3) return 4;

    std::scoped_lock moved_source(first, second);
    std::scoped_lock moved_target(static_cast<std::scoped_lock &&>(moved_source));
    if (locks != 5) return 5;
    if (unlocks != 3) return 6;
    return 0;
}

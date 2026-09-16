// EXPECT_EXIT: 0
// EXPECT_COMPILE_ARGS: -Werror
#include <atomic>

int main() {
    std::atomic<unsigned char> byte(2);
    std::atomic<unsigned short> half(1000);
    std::atomic<unsigned int> word(100000);
    std::atomic<unsigned long long> wide(0x123456789abcdef0ull);
    if (byte++ != 2 || ++byte != 4 || byte.load() != 4) return 1;
    if (half-- != 1000 || --half != 998) return 2;
    word.store(123, std::memory_order_release);
    if (word.load(std::memory_order_acquire) != 123) return 3;
    if (wide++ != 0x123456789abcdef0ull || --wide != 0x123456789abcdef0ull) return 4;
    return 0;
}

#include <type_traits>

int main() {
    auto safe = [](int value) noexcept { return value + 1; };
    auto throwing = [](int value) { return value + 2; };
    int offset = 3;
    auto captured = [offset](int value) noexcept { return value + offset; };
    static_assert(noexcept(safe(1)), "lambda call retains noexcept");
    static_assert(noexcept(captured(1)), "captured lambda retains noexcept");
    static_assert(!noexcept(throwing(1)), "ordinary lambda may throw");
    using Pointer = int (*)(int) noexcept;
    static_assert(std::is_same<decltype(+safe), Pointer>::value,
                  "conversion preserves noexcept function type");
    Pointer pointer = safe;
    return pointer(2) != 3 || captured(2) != 5 || throwing(2) != 4;
}

#include <type_traits>

int main() {
    auto safe = [](int value) noexcept(sizeof(value) == sizeof(int)) { return value; };
    auto risky = [](int value) noexcept(false) { return value; };
    auto generic = [](auto value) noexcept(sizeof(value) == sizeof(int)) { return value; };
    static_assert(noexcept(safe(1)), "parameter scope in specification");
    static_assert(!noexcept(risky(1)), "false specification");
    static_assert(noexcept(generic(1)), "nonthrowing specialization");
    static_assert(!noexcept(generic(1.0)), "throwing specialization");
    using Safe = int (*)(int) noexcept;
    using Risky = int (*)(int);
    static_assert(std::is_same<decltype(+safe), Safe>::value, "safe conversion");
    static_assert(std::is_same<decltype(+risky), Risky>::value, "throwing conversion");
    return safe(2) != 2 || risky(3) != 3 || generic(4) != 4;
}

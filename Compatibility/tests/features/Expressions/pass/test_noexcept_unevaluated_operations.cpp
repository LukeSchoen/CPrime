#include <type_traits>

int calls;
int safe(int value = 0) noexcept { ++calls; return value; }
int risky(int value = 0) { ++calls; return value; }
int defaulted(int value = risky()) noexcept { return value; }
struct Object {
    Object() noexcept {}
    Object(int) {}
    int good() const noexcept { return 1; }
    int bad() const { return 2; }
};
struct Cleanup {
    Cleanup() noexcept {}
    ~Cleanup() noexcept(false) {}
};
int main() {
    Object object;
    int (*safe_pointer)(int) noexcept = safe;
    int (*risky_pointer)(int) = risky;
    static_assert(noexcept(1 + 2), "arithmetic");
    static_assert(noexcept(safe()), "safe call");
    static_assert(!noexcept(risky()), "throwing call");
    static_assert(noexcept(safe_pointer(1)), "safe pointer");
    static_assert(!noexcept(risky_pointer(1)), "throwing pointer");
    static_assert(noexcept(object.good()), "safe member");
    static_assert(!noexcept(object.bad()), "throwing member");
    static_assert(!noexcept(safe(risky())), "argument evaluated");
    static_assert(!noexcept(defaulted()), "default argument evaluated");
    static_assert(noexcept(defaulted(1)), "unused default argument");
    static_assert(noexcept(sizeof(risky())), "sizeof operand excluded");
    static_assert(noexcept(noexcept(risky())), "nested query excluded");
    static_assert(!noexcept(false && risky()), "potential evaluation");
    static_assert(!noexcept(true ? safe() : risky()), "both conditional arms");
    static_assert(!noexcept(throw 1), "throw expression");
    static_assert(noexcept(Object()), "safe constructor");
    static_assert(!noexcept(Object(1)), "throwing constructor");
    static_assert(!noexcept(Cleanup()), "temporary destructor");
    static_assert(std::is_same<decltype(noexcept(safe())), bool>::value,
                  "result type is bool");
    return calls != 0;
}

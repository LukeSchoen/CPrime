#include <functional>
struct Counter { int value; int add(int n) { return value += n; } };
template<class F, class... Args> int apply(F callable, Args... args) {
    auto task = [=]() mutable -> int {
        return std::invoke(std::move(callable), std::move(args)...);
    };
    return task();
}
int main() { Counter counter = {3}; return apply(&Counter::add, &counter, 4) != 7; }

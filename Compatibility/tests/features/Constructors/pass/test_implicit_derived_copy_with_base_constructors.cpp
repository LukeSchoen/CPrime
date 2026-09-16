#include <utility>
int copies, moves, conversions;
struct Base {
    int value;
    Base() : value(7) {}
    Base(const Base &other) : value(other.value) { ++copies; }
    Base(Base &&other) : value(other.value) { other.value = 0; ++moves; }
};
struct Derived : Base {
    int extra;
    Derived() : extra(9) {}
    Derived(const Base &base) : Base(base), extra(1) { ++conversions; }
    Derived(Base &&base) : Base(std::move(base)), extra(2) { ++conversions; }
};
int main() {
    Derived source;
    Derived moved(std::move(source));
    if (moved.value != 7 || moved.extra != 9 || moves != 1 || conversions) return 1;
    Derived copied(moved);
    return copied.value != 7 || copied.extra != 9 || copies != 1 || conversions;
}

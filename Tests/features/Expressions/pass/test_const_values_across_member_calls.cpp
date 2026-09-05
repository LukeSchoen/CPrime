// EXPECT_COMPILE_ARGS: -Werror
struct Counter {
    int calls;
    int next() { ++calls; return 3; }
};
template<class C> double compute(C& counter) {
    const int weight = 4;
    const double offset = 1.5;
    auto next = [&]() { return counter.next(); };
    return weight * next() + offset * next();
}
int main() {
    Counter counter = {0};
    const int value = 7;
    const volatile int sampled = 2;
    int result = value * counter.next() + sampled * counter.next();
    if (result != 27 || value != 7 || sampled != 2) return 1;
    if (compute(counter) != 16.5 || counter.calls != 4) return 2;
    return 0;
}

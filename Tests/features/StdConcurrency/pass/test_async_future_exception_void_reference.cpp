// EXPECT_EXIT: 0
#include <future>
struct Failure { int value; Failure(int n) : value(n) {} };
int fail() { throw Failure(37); }
int number = 0;
void set_number() { number = 19; }
int &number_reference() { return number; }
int main() {
    auto failed = std::async(std::launch::async, fail);
    try { failed.get(); return 1; }
    catch (const Failure &error) { if (error.value != 37) return 2; }
    if (failed.valid()) return 3;
    std::future<void> empty = std::async(std::launch::async, set_number);
    empty.get();
    if (number != 19 || empty.valid()) return 4;
    std::future<int &> reference = std::async(std::launch::deferred, number_reference);
    reference.get() = 29;
    return number == 29 ? 0 : 5;
}

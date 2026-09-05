// EXPECT_EXIT: 0
#include <future>
#include <functional>

unsigned worker_id() { return std::this_thread::get_id(); }
struct Calculator {
    int total;
    Calculator() : total(0) {}
    int add(int amount) { total += amount; return total; }
};
int main() {
    std::future<unsigned> worker = std::async(std::launch::async, worker_id);
    if (!worker.valid()) return 1;
    unsigned id = worker.get();
    if (worker.valid() || id == std::this_thread::get_id()) return 2;
    Calculator calculator;
    std::future<int> deferred = std::async(std::launch::deferred, &Calculator::add, &calculator, 7);
    if (calculator.total != 0 || !deferred.valid()) return 3;
    std::future<int> moved(std::move(deferred));
    if (deferred.valid() || !moved.valid()) return 4;
    moved.wait();
    if (calculator.total != 7 || moved.get() != 7) return 5;
    {
        auto discarded = std::async(std::launch::deferred, &Calculator::add, &calculator, 100);
    }
    if (calculator.total != 7) return 6;
    {
        auto joined = std::async(std::launch::async, &Calculator::add, &calculator, 11);
    }
    if (calculator.total != 18) return 7;
    return 0;
}

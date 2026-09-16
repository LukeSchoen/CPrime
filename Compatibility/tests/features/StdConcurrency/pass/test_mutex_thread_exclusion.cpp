// EXPECT_EXIT: 0
// EXPECT_COMPILE_ARGS: -Werror
#include <mutex>
#include <thread>

struct Shared {
    std::mutex lock;
    unsigned count;
    unsigned worker_id;
    Shared() : count(0), worker_id(0) {}
};
struct Worker {
    Shared *shared;
    explicit Worker(Shared *value) : shared(value) {}
    void operator()() {
        shared->worker_id = std::this_thread::get_id();
        for (int i = 0; i != 5000; ++i) {
            std::lock_guard<std::mutex> lock(shared->lock);
            ++shared->count;
        }
    }
};
int main() {
    Shared shared;
    Worker job(&shared);
    std::thread worker(job);
    if (!worker.joinable()) return 1;
    for (int i = 0; i != 5000; ++i) {
        std::unique_lock<std::mutex> lock(shared.lock);
        ++shared.count;
        lock.unlock();
        if (lock.owns_lock()) return 2;
    }
    worker.join();
    if (worker.joinable() || shared.count != 10000) return 3;
    if (!shared.worker_id || shared.worker_id == std::this_thread::get_id()) return 4;
    shared.lock.lock();
    if (shared.lock.try_lock()) return 5;
    shared.lock.unlock();
    std::recursive_mutex recursive;
    recursive.lock();
    if (!recursive.try_lock()) return 6;
    recursive.unlock();
    recursive.unlock();
    return 0;
}

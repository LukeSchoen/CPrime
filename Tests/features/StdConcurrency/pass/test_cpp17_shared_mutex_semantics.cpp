// EXPECT_EXIT: 0
// EXPECT_COMPILE_ARGS: -std=c++17
// Coverage: shared_mutex counts readers instead of excluding them. Two shared
// holders coexist, a writer is refused while either holds the lock, and a
// writer waiting on a reader enters only after the reader releases it. The
// former plain-mutex implementation fails the second shared acquisition.
#include <shared_mutex>
#include <thread>

static std::shared_mutex guard;
static volatile int writer_started = 0;
static volatile int writer_entered = 0;

static void spin_until(volatile int *flag) {
    for (volatile long i = 0; i < 20000000 && !*flag; ++i) { }
}

static void writer_body() {
    writer_started = 1;
    guard.lock();
    writer_entered = 1;
    guard.unlock();
}

int main() {
    if (!guard.try_lock_shared()) return 1;
    if (!guard.try_lock_shared()) return 2;   /* readers share the lock */
    if (guard.try_lock()) return 3;           /* a writer is refused */
    guard.unlock_shared();
    guard.unlock_shared();

    if (!guard.try_lock()) return 4;          /* a free lock is takeable */
    if (guard.try_lock_shared()) return 5;    /* a writer excludes readers */
    guard.unlock();
    if (!guard.try_lock_shared()) return 6;
    guard.unlock_shared();

    /* The writer blocks until the shared holder releases the lock. */
    guard.lock_shared();
    std::thread writer(writer_body);
    spin_until(&writer_started);
    for (volatile long i = 0; i < 5000000; ++i) { }
    if (writer_entered) return 7;
    guard.unlock_shared();
    writer.join();
    if (!writer_entered) return 8;

    std::shared_lock<std::shared_mutex> shared(guard);
    if (!shared.owns_lock()) return 9;
    shared.unlock();
    if (shared.owns_lock()) return 10;
    if (!shared.try_lock()) return 11;        /* a released lock can be retaken */
    std::shared_lock<std::shared_mutex> moved(
        static_cast<std::shared_lock<std::shared_mutex> &&>(shared));
    if (!moved.owns_lock()) return 12;
    if (shared.owns_lock()) return 13;
    return 0;
}

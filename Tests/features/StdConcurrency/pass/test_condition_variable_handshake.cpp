// EXPECT_EXIT: 0
#include <mutex>
#include <condition_variable>
#include <thread>
struct State {
    std::mutex mutex;
    std::condition_variable changed;
    bool ready, released;
    int result;
    State() : ready(false), released(false), result(0) {}
};
struct Worker {
    State *state;
    explicit Worker(State *input) : state(input) {}
    void operator()() {
        std::unique_lock<std::mutex> lock(state->mutex);
        state->ready = true;
        state->changed.notify_one();
        while (!state->released) state->changed.wait(lock);
        state->result = 29;
    }
};
int main() {
    State state;
    Worker job(&state);
    std::thread worker(job);
    {
        std::unique_lock<std::mutex> lock(state.mutex);
        while (!state.ready) state.changed.wait(lock);
        if (state.result != 0) return 1;
        state.released = true;
        state.changed.notify_all();
    }
    worker.join();
    return state.result == 29 ? 0 : 2;
}

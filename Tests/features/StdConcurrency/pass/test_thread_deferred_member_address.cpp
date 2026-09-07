#include <thread>
struct Worker {
 int value;
 std::thread worker;
 Worker();
 void task();
};
Worker::Worker() : value(0), worker(&Worker::task, this) {}
void Worker::task() { value = 7; }
int main() { Worker object; object.worker.join(); return object.value != 7; }

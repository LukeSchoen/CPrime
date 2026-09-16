// EXPECT_EXIT: 0
#include <future>
#include <functional>

template<class T> struct Worker {
  struct Node { int value; };
  Node node;
  Node *run(T &value, int increment) {
    node.value = value + increment;
    return &node;
  }
  int invoke(T &value) {
    auto result = std::async(std::launch::deferred, &Worker::run,
                             this, std::ref(value), 3);
    return result.get()->value;
  }
};
int main() {
  Worker<int> worker;
  int value = 4;
  return worker.invoke(value) == 7 ? 0 : 1;
}

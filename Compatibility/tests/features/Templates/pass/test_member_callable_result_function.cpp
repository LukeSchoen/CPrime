#include <functional>
struct Scheduler {
  int add(int, const std::function<long long()>&) { return 1; }
  int add(int, const std::function<void()>&) { return 2; }
  template<class Callable> int add(int time, const Callable& payload) {
    using Result = decltype(payload());
    return add(time, std::function<Result()>([=]() -> auto { return payload(); }));
  }
};
int main() { Scheduler s; return s.add(0, [](){ return 5LL; }) == 1 ? 0 : 1; }

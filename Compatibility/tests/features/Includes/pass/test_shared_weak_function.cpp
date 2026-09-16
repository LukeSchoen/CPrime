#include <memory>
#include <functional>
static int destroyed;
struct Object { int value; Object(int v) : value(v) {} ~Object() { ++destroyed; } };
int main() {
  std::weak_ptr<Object> weak;
  {
    auto a = std::make_shared<Object>(7);
    weak = a;
    auto b = a;
    if (a.use_count() != 2 || b->value != 7) return 1;
    {
      auto c = weak.lock();
      if (!c || c.use_count() != 3) return 2;
    }
    std::function<int(int)> f = [a](int n) { return a->value + n; };
    auto g = f;
    f = nullptr;
    if (g(5) != 12 || weak.expired()) return 3;
  }
  if (!weak.expired() || weak.lock() || destroyed != 1) return 4;
  return 0;
}

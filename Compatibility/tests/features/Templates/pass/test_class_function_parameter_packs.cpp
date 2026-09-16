#include <functional>
template<class... Args> struct Calls {
  int first(Args... args) { return sum(args...); }
  int second(Args... values) { return sum(values...); }
  static int sum() { return 7; }
  static int sum(int x, long y) { return x + int(y); }
};
int main() {
  Calls<> empty;
  Calls<int,long> pair;
  std::function<void()> f = [](){};
  std::function<int(int,long)> g = [](int x,long y){return x+int(y);};
  f();
  return empty.first()!=7 || empty.second()!=7 || pair.first(3,4)!=7
    || pair.second(5,6)!=11 || g(8,9)!=17;
}

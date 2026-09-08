namespace API {
  int f(int) { return 1; }
  inline namespace v1 {
    int f(double) { return 2; }
    struct Value {};
    template<class T> int g(T) { return 3; }
  }
  inline namespace v2 {
    int inspect(Value) { return 4; }
  }
  template<class T> int g(T, T) { return 5; }
}
using API::g;
int main() {
  API::Value value;
  return API::f(1) != 1 || API::f(1.0) != 2
      || g(1) != 3 || g(1, 2) != 5 || inspect(value) != 4;
}

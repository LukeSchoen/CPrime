template<class T> int identity(T value) { return value; }
template<class T> int apply(int (*fn)(T), T value) { return fn(value); }
struct Functions { static int run(int n) { return n + 1; } static void run(); };
template<class T> int invoke(int (*fn)(T)) { return fn(3); }
int main() {
  return apply<int>(identity, 5) != 5 || apply(identity<int>, 6) != 6
      || apply(identity, 7) != 7 || invoke(&Functions::run) != 4;
}

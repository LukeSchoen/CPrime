namespace lib {
  template<class T> struct Limits { static T max() { return T(17); } };
}
int main() {
  int a = (lib::Limits<int>::max)();
  int b = (int)(4.9);
  return a != 17 || b != 4;
}

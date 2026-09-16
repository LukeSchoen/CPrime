// EXPECT_EXIT: 0
struct Value {
  template<class T> static int read(T*, short = 0) { return 7; }
  template<class T> static int read(T, int = 0) { return 3; }
  template<class T> int member(T) { return 3; }
  template<class T> int member(T*, int = 0) { return 7; }
  static int ordinary(int*) { return 9; }
  template<class T> static int ordinary(T*) { return 3; }
};
int main() { Value value; int n = 0; return Value::read(&n) != 7 || value.member(&n) != 7
    || Value::ordinary(&n) != 9; }

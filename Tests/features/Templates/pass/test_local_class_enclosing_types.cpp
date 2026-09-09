// EXPECT_EXIT: 0
struct BaseType { typedef char Number; };
template<int N> int check() {
  class Value { public: int number; };
  enum Choice { selected = N };
  typedef short Number;
  struct Local {
    int read() { Value value; value.number = N; Choice c = selected;
      Number n = 2; return value.number + (int)c + sizeof(n); }
  };
  struct Derived : BaseType { int size() { return sizeof(Number); } };
  struct Own : BaseType { typedef int Number; int size() { return sizeof(Number); } };
  Local local; Derived derived; Own own;
  if (own.size() != sizeof(int)) return 99;
  return local.read() + derived.size();
}
int main() { return check<7>() != 17 || check<-3>() != -3; }

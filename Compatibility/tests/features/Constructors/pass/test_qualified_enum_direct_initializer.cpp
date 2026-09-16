// EXPECT_EXIT: 0
struct A { enum E { value = 3 }; int number; A(E input) : number(input) {} };
struct B { enum E { value = 5 }; int number; B(E input) : number(input) {} };
struct Combined {
  int total;
  Combined(A a, B b, A c) : total(a.number + b.number + c.number) {}
};
Combined global(A(A::value), B(B::value), A(A::value));
Combined declaration(A(named), B(other), A(last));
int main() {
  Combined local(A(A::value), B(B::value), A(A::value));
  return global.total != 11 || local.total != 11;
}

struct C {
  int i;
  C() : i(1) {}
  int f() {
    struct D {
      int i;
      D() : i(2) {}
      int g() { return i; }
    } d;
    return d.g();
  }
};

int main()
{
  C c;
  return c.f() != 2;
}

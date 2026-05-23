class A {
public:
  int x;
protected:
  int y;
private:
  int z;
};

struct B {
public:
  int x;
private:
  int y;
protected:
  int z;
};

int main(void) {
  class A a;
  struct B b;
  a.x = 1;
  b.x = 2;
  return a.x + b.x - 3;
}
struct Base { static const int count = 17; };
struct Derived : Base { int read(int count); };
int Derived::read(int count) { return count + 1; }
int main() { Derived d; return d.read(4) != 5; }

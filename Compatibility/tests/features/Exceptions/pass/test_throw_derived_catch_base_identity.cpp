// EXPECT_EXIT: 0
struct Base { int value; Base(int n) : value(n) {} virtual ~Base() {} };
struct Derived : Base { Derived() : Base(17) {} };
int caught;
void run() {
    try { throw Derived(); }
    catch (const Base& base) { caught = base.value; }
}
int main() { run(); return caught != 17; }

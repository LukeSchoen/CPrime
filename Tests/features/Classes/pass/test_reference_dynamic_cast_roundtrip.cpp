// EXPECT_EXIT: 0
struct Base { virtual ~Base() {} };
struct Derived : Base { int value; Derived() : value(6) {} };
template<class T> void change(Base& base) { dynamic_cast<T&>(base).value += 3; }
int main() {
    Derived object; Base& base = object;
    change<Derived>(base);
    return &dynamic_cast<Derived&>(base) != &object || object.value != 9;
}

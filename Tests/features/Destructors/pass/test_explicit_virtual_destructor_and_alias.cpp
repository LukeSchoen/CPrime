// EXPECT_EXIT: 0
int base_count, derived_count;
struct Base { typedef Base Alias; virtual ~Base() { ++base_count; } };
struct Derived : Base { ~Derived() { ++derived_count; } };
int main() {
    Base *first = new Derived;
    first->~Alias();
    if (base_count != 1 || derived_count != 1) return 1;
    Derived *second = new Derived;
    second->Base::~Base();
    return base_count != 2 || derived_count != 1;
}

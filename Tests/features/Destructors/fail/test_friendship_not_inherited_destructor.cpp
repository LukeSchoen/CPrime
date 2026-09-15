// EXPECT_COMPILE_FAIL: 1
struct Friend;
struct Value {
    friend struct Friend;
private:
    ~Value() {}
};
struct Friend {};
struct Derived : Friend {
    static void destroy(Value *value) { value->~Value(); }
};
int main() { Derived::destroy(nullptr); }

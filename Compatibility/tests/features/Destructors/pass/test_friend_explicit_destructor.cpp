#include <new>
struct Friend;
struct Value {
    friend struct Friend;
private:
    ~Value() {}
};
struct Friend {
    static void exercise() {
        Value *value = new Value;
        value->~Value();
        ::operator delete(value);
    }
};
int main() { Friend::exercise(); }

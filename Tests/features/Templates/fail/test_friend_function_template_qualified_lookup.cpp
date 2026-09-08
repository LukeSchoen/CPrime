// EXPECT_COMPILE_FAIL: 1
namespace Example {
struct Value {
    template<class T> friend int hidden(Value, T) { return 1; }
};
}
int instantiate() { return hidden(Example::Value(), 0); }
int main() { Example::Value value; return Example::hidden(value, 0); }

// EXPECT_COMPILE_FAIL: 1
template<class T> class Value {
    ~Value() {}
public:
    Value() {}
};
int main() { Value<int> value; }

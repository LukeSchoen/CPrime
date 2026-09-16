struct Value {
    int number;
    constexpr const int &get() const { return number; }
};
constexpr Value value{9};
constexpr int result = value.get();
static_assert(result == 9);
static_assert(&value.get() == &value.number);
static_assert(Value{4}.get() == 4);
int calls;
Value make() { ++calls; return Value{7}; }
int main() {
    Value local{3};
    return result != 9 || &value.get() != &value.number
        || &local.get() != &local.number || make().get() != 7 || calls != 1;
}

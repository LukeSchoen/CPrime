// EXPECT_EXIT: 0
int assignments;
struct Value {
    int value;
    Value& operator=(Value const& source) { value = source.value; ++assignments; return *this; }
};
struct Source {
    Value value;
    operator Value&() { return value; }
};
int main() {
    Value destination = {3};
    Source source = {{23}};
    destination = source;
    return assignments != 1 || destination.value != 23;
}

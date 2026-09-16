// EXPECT_EXIT: 0
struct Value { int member; int function() { return 7; } };
struct SafeBool {
    bool valid;
    operator int Value::*() const { if (valid) return &Value::member; return 0; }
};
int accept(bool value) { return value ? 11 : 17; }
int reference(bool const& value) { return value ? 23 : 29; }
int main() {
    int Value::* data = &Value::member;
    int (Value::* function)() = &Value::function;
    SafeBool yes = {true}, no = {false};
    return accept(data) != 11 || accept(function) != 11
        || accept(yes) != 11 || accept(no) != 17
        || reference(yes) != 23 || reference(no) != 29;
}

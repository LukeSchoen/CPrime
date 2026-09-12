static int copies, destroyed;
struct Value {
    int value;
    Value(int value) : value(value) {}
    Value(const Value& source) : value(source.value) { ++copies; }
    ~Value() { ++destroyed; }
};
static int normal(Value value) { return value.value; }
static void exceptional(Value value) { throw value.value; }
static void unnamed(Value) {}
int main() {
    Value original(173);
    // The by-value parameter is a temporary of this full expression and is
    // destroyed when that full expression ends, so the whole condition below
    // still observes it alive.  See
    // Tests/features/Destructors/pass/test_value_parameter_temporary_outlives_call.cpp.
    if (normal(original) != 173 || copies != 1 || destroyed != 0) return 1;
    if (copies != 1 || destroyed != 1) return 4;
    try { exceptional(original); }
    catch (int value) { if (value != 173 || copies != 2 || destroyed != 2) return 2; }
    unnamed(original);
    return copies != 3 || destroyed != 3 || original.value != 173;
}

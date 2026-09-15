struct Value { int number; };
constexpr int read(const Value &value) { return value.number; }
constexpr int global = Value{4}.number;
constexpr int global_call = read(Value{9});
struct Array { int values[2]; };
constexpr int element = Array{{3, 8}}.values[1];
constexpr int unevaluated = sizeof(Array{}.values[100]);
int calls;
int next_value() { ++calls; return 6; }
int dynamic = Value{next_value()}.number;
int main() {
    constexpr int local = Value{7}.number;
    constexpr int local_call = read(Value{8});
    static_assert(global == 4 && global_call == 9);
    static_assert(element == 8 && unevaluated == sizeof(int));
    static_assert(local == 7 && local_call == 8);
    return local != 7 || local_call != 8 || dynamic != 6 || calls != 1;
}

int calls;
int source() { ++calls; return 7; }
struct Value { float number = source(); };
struct Outer { int prefix; Value value; };
int main() {
    Outer object{3, {}};
    float array[2]{16777216, -16777216};
    return object.value.number != 7 || calls != 1 || array[0] != -array[1];
}

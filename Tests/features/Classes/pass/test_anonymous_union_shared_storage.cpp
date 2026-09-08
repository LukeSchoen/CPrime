// EXPECT_EXIT: 0
static union { int global_value; unsigned global_bits; };
int main() {
    union { int value; unsigned bits; };
    value = 37;
    global_value = 19;
    if (bits != 37 || global_bits != 19) return 1;
    bits = 41;
    global_bits = 23;
    return value != 41 || ::global_value != 23;
}

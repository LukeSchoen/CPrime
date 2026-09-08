// EXPECT_EXIT: 0
struct Amount { int value; };
struct Value {
    int value;
    Value& operator+=(Value rhs) { value += rhs.value; return *this; }
};
Value& operator+=(Value& left, Amount right) { left.value += 2 * right.value; return left; }
Value& operator<<=(Value& left, Amount right) { left.value <<= right.value; return left; }
int main() {
    Value left = {3}, other = {5};
    Amount amount = {2};
    Value& result = (left += amount);
    if (&result != &left || left.value != 7) return 1;
    left += other;
    left <<= amount;
    return left.value != 48;
}

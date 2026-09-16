// EXPECT_EXIT: 0
int evaluations, conversions;
int unknown;
int folded_and = __builtin_constant_p(unknown == 42 && false);
int folded_or = __builtin_constant_p(unknown == 42 || true);
struct Operand { int value; };
Operand make(int value) { ++evaluations; Operand result = {value}; return result; }
Operand operator&&(Operand left, Operand right) { Operand result = {left.value + right.value}; return result; }
int operator||(Operand left, Operand right) { return left.value + right.value + 10; }
struct Boolean {
    bool value;
    explicit operator bool() const { ++conversions; return value; }
};
Boolean boolean(bool value) { ++evaluations; Boolean result = {value}; return result; }
int operator||(Boolean, int*) { return 71; }
int choose(bool) { return 1; }
int choose(int) { return 2; }
int main() {
    Operand result = make(0) && make(3) && make(5);
    if (result.value != 8 || evaluations != 3) return 1;
    if ((make(1) || make(2)) != 13 || evaluations != 5) return 2;
    evaluations = conversions = 0;
    bool value = boolean(false) && boolean(true);
    if (value || evaluations != 1 || conversions != 1) return 3;
    value = boolean(true) || boolean(false);
    if (!value || evaluations != 2 || conversions != 2) return 4;
    value = false && boolean(true);
    if (value || evaluations != 2) return 5;
    value = false || boolean(true);
    if (!value || evaluations != 3 || conversions != 3) return 6;
    int side_effect = 0;
    value = false && ++side_effect || true;
    return !value || side_effect || choose(1 && 2) != 1 || !folded_and || !folded_or;
}

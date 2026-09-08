// EXPECT_EXIT: 0
struct Left { int value; operator int() const { return value; } };
struct Right { int value; operator int() const { return value + 10; } };
int choose(bool first, Left left, Right right) { return first ? left : right; }
int main() {
    Left left = {3}; Right right = {5};
    return choose(true, left, right) != 3 || choose(false, left, right) != 15;
}

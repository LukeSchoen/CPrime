// EXPECT_EXIT: 0
// EXPECT_COMPILE_ARGS: -Werror
struct Item { int value; Item(int input) : value(input) {} };
int main() {
    Item *item = new Item(17);
    int *values = new int[2];
    values[0] = item->value;
    values[1] = 25;
    int result = values[0] + values[1];
    delete item;
    delete[] values;
    return result == 42 ? 0 : 1;
}

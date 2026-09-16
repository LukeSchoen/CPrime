// EXPECT_COMPILE_ARGS: -Werror
struct Item {
    int value;
    Item() : value(42) {}
};
Item* allocate_item() { return new Item; }
void subsequent_function() { int local; }
#include <cstdlib>

int main() {
    Item* item = allocate_item();
    int result = item->value != 42;
    delete item;
    void* ordinary_allocation = malloc(32);
    if (!ordinary_allocation) return 2;
    free(ordinary_allocation);
    return result;
}

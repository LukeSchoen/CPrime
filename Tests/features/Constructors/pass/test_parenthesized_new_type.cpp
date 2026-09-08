// EXPECT_EXIT: 0
int twice(int n) { return 2 * n; }
struct Object { int value; Object() : value(7) {} };
int main() {
    int *integers = new (int[5]);
    Object *objects = new (Object[2]);
    int (**function)(int) = new (int (*)(int))(&twice);
    integers[4] = 13;
    int result = integers[4] != 13 || objects[1].value != 7 || (*function)(3) != 6;
    delete[] integers; delete[] objects; delete function;
    return result;
}

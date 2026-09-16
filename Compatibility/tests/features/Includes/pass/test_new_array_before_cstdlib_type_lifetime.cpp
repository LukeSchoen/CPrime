// EXPECT_COMPILE_ARGS: -Werror
// Unity translation units can encounter allocation before a later CRT include.
float* allocate_values() { return new float[16]; }
void subsequent_function() { int local; }
#include <cstdlib>

int main() {
    float* values = allocate_values();
    values[0] = 3.0f;
    values[15] = 7.0f;
    int result = values[0] != 3.0f || values[15] != 7.0f;
    delete[] values;
    void* ordinary_allocation = malloc(32);
    if (!ordinary_allocation) return 2;
    free(ordinary_allocation);
    return result;
}

#include <new>
#include <stdlib.h>
int released;
void operator delete(void* pointer) noexcept { ++released; free(pointer); }
int main() {
    int* scalar = new int(42);
    delete scalar;
    int* array = new int[3];
    delete[] array;
    void* memory = malloc(16);
    ::operator delete(memory, (std::size_t)16);
    return released != 3;
}

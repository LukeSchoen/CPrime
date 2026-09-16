// EXPECT_EXIT: 0
#include <new>
#include <stdlib.h>
int array_allocations, constructions, scalar_allocations;
void* operator new[](std::size_t size) {
    ++array_allocations;
    return malloc(size ? size : 1);
}
struct Empty { Empty() { ++constructions; } };
struct Node {
    int value;
    void* operator new(std::size_t size, char*& cursor) {
        void* result = cursor;
        cursor += size;
        return result;
    }
    Node() : value(23) {}
};
struct Derived : Node { int extra; Derived() : extra(29) {} };
struct Allocated {
    int value;
    void* operator new(std::size_t size) { ++scalar_allocations; return malloc(size); }
    Allocated() : value(31) {}
};
alignas(Derived) char storage[sizeof(Derived) * 2];
int main() {
    Empty* empty = new Empty[0];
    if (array_allocations != 1 || constructions != 0) return 1;
    delete[] empty;
    char* cursor = storage;
    Derived* derived = new (cursor) Derived;
    if (cursor != storage + sizeof(Derived) || derived->value != 23 || derived->extra != 29) return 2;
    Allocated* allocated = new Allocated;
    if (scalar_allocations != 1 || allocated->value != 31) return 3;
    delete allocated;
    allocated = ::new Allocated;
    if (scalar_allocations != 1 || allocated->value != 31) return 4;
    delete allocated;
    return 0;
}

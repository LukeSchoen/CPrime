#include <new>
int released, destroyed;
struct Empty {
    static void operator delete[](void* pointer) {
        ++released;
        ::operator delete(pointer);
    }
};
struct Element {
    ~Element() noexcept(false) { ++destroyed; if (destroyed == 1) throw 7; }
    static void operator delete[](void* pointer) {
        ++released;
        ::operator delete(pointer);
    }
};
int main() {
    Empty (*array)[2] = new Empty[2][2];
    delete[] array;
    if (released != 1) return 1;
    Element* elements = new Element[2];
    try { delete[] elements; return 2; }
    catch (int value) { if (value != 7) return 3; }
    return released != 2 || destroyed != 2;
}

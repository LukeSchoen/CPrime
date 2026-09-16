static __SIZE_TYPE__ allocated, released;
struct Element {
    int value;
    static void* operator new[](__SIZE_TYPE__ size) {
        allocated = size;
        return ::operator new[](size);
    }
    static void operator delete[](void* pointer, __SIZE_TYPE__ size) {
        released = size;
        ::operator delete[](pointer);
    }
};
int main() {
    Element* array = new Element[3];
    delete[] array;
    ::operator delete(::operator new(4));
    return !released || released != allocated;
}

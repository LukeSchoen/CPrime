#include <new>
#include <stdlib.h>

static std::size_t allocated, released;
static int destroyed;

struct Trivial {
    int value;
    static void* operator new[](std::size_t size) {
        allocated = size;
        return malloc(size);
    }
    static void operator delete[](void* pointer, std::size_t size) noexcept {
        released = size;
        free(pointer);
    }
};

struct Throwing {
    int value;
    ~Throwing() noexcept(false) { if (++destroyed == 1) throw 7; }
    static void* operator new[](std::size_t size) {
        allocated = size;
        return malloc(size);
    }
    static void operator delete[](void* pointer, std::size_t size) noexcept {
        released = size;
        free(pointer);
    }
    static void operator delete(void* pointer, std::size_t size) noexcept {
        released = size;
        free(pointer);
    }
};

int main() {
    Trivial* trivial = new Trivial[3];
    delete[] trivial;
    if (!released || released != allocated) return 1;
    released = 0;
    Throwing* array = new Throwing[3];
    try { delete[] array; return 2; } catch (int value) { if (value != 7) return 3; }
    if (destroyed != 3 || released != allocated) return 4;
    destroyed = 0;
    released = 0;
    Throwing* scalar = new Throwing;
    try { delete scalar; return 5; } catch (int value) { if (value != 7) return 6; }
    if (destroyed != 1 || released != sizeof(Throwing)) return 7;
}

#include <new>
#include <stdlib.h>

__attribute__((weak)) void* operator new(std::size_t size) {
    void* pointer = malloc(size ? size : 1);
    if (!pointer) throw std::bad_alloc();
    return pointer;
}
__attribute__((weak)) void* operator new[](std::size_t size) {
    return ::operator new(size);
}
__attribute__((weak)) void* operator new(std::size_t size, const std::nothrow_t&) noexcept {
    try { return ::operator new(size); }
    catch (...) { return 0; }
}
__attribute__((weak)) void* operator new[](std::size_t size, const std::nothrow_t&) noexcept {
    try { return ::operator new[](size); }
    catch (...) { return 0; }
}

__attribute__((weak)) void operator delete(void* pointer) noexcept {
    free(pointer);
}
__attribute__((weak)) void operator delete[](void* pointer) noexcept {
    ::operator delete(pointer);
}
__attribute__((weak)) void operator delete(void* pointer, std::size_t) noexcept {
    ::operator delete(pointer);
}
__attribute__((weak)) void operator delete[](void* pointer, std::size_t) noexcept {
    ::operator delete[](pointer);
}
__attribute__((weak)) void operator delete(void* pointer, const std::nothrow_t&) noexcept {
    ::operator delete(pointer);
}
__attribute__((weak)) void operator delete[](void* pointer, const std::nothrow_t&) noexcept {
    ::operator delete[](pointer);
}

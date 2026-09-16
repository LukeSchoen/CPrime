#include <new>
#include <malloc.h>
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

/* Over-aligned allocation is served by the CRT's aligned heap, so the
   matching delete overloads must return the block there. */
__attribute__((weak)) void* operator new(std::size_t size, std::align_val_t alignment) {
    void* pointer = _aligned_malloc(size ? size : 1, (std::size_t)alignment);
    if (!pointer) throw std::bad_alloc();
    return pointer;
}
__attribute__((weak)) void* operator new[](std::size_t size, std::align_val_t alignment) {
    return ::operator new(size, alignment);
}
__attribute__((weak)) void* operator new(std::size_t size, std::align_val_t alignment,
                                        const std::nothrow_t&) noexcept {
    try { return ::operator new(size, alignment); }
    catch (...) { return 0; }
}
__attribute__((weak)) void* operator new[](std::size_t size, std::align_val_t alignment,
                                          const std::nothrow_t&) noexcept {
    try { return ::operator new[](size, alignment); }
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
__attribute__((weak)) void operator delete(void* pointer, std::align_val_t) noexcept {
    _aligned_free(pointer);
}
__attribute__((weak)) void operator delete[](void* pointer, std::align_val_t) noexcept {
    _aligned_free(pointer);
}
__attribute__((weak)) void operator delete(void* pointer, std::size_t, std::align_val_t) noexcept {
    _aligned_free(pointer);
}
__attribute__((weak)) void operator delete[](void* pointer, std::size_t, std::align_val_t) noexcept {
    _aligned_free(pointer);
}

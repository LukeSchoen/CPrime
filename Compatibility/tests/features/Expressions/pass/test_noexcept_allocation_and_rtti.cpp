#include <new>
#include <typeinfo>

struct Base { virtual ~Base() {} };
struct Derived : Base {};
struct ThrowingCleanup { ~ThrowingCleanup() noexcept(false) {} };
int main() {
    Base *base = nullptr;
    int *integer = nullptr;
    ThrowingCleanup *cleanup = nullptr;
    alignas(int) unsigned char storage[sizeof(int)];
    static_assert(!noexcept(new int), "allocation can throw");
    static_assert(!noexcept(new int[2]), "array allocation can throw");
    static_assert(noexcept(new (storage) int), "nonallocating placement");
    static_assert(noexcept(new (std::nothrow) int), "nothrow allocation");
    static_assert(noexcept(delete integer), "ordinary deletion");
    static_assert(!noexcept(delete cleanup), "throwing destructor");
    static_assert(noexcept(dynamic_cast<Derived*>(base)), "pointer cast");
    static_assert(!noexcept(dynamic_cast<Derived&>(*base)), "reference cast");
    static_assert(noexcept(typeid(int)), "type operand");
    static_assert(noexcept(typeid(*integer)), "nonpolymorphic operand");
    static_assert(!noexcept(typeid(*base)), "polymorphic operand");
    return 0;
}

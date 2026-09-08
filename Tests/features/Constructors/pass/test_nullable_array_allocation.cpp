// EXPECT_EXIT: 0
#include <new>
int allocations, constructions, destructions;
void* operator new[](std::size_t) noexcept { ++allocations; return 0; }
struct Object {
    Object() { ++constructions; }
    ~Object() { ++destructions; }
};
int main() {
    Object* objects = new Object[3];
    return objects != 0 || allocations != 1 || constructions || destructions;
}

// EXPECT_EXIT: 0
#include <new>

int throwing_allocations, nothrow_allocations, placements, placement_bytes, placement_tags;
int constructions, destructions;

void* operator new[](std::size_t) noexcept { ++throwing_allocations; return 0; }
void* operator new[](std::size_t, std::nothrow_t const&) noexcept { ++nothrow_allocations; return 0; }

struct Tag { int value; };
void* operator new[](std::size_t bytes, Tag tag, void* address) {
    ++placements;
    placement_bytes = (int)bytes;
    placement_tags += tag.value;
    return address;
}

struct Object {
    int payload;
    Object() : payload(7) { ++constructions; }
    ~Object() { ++destructions; }
};

int main() {
    Object* nothing = new (std::nothrow) Object[3];
    if (nothing || nothrow_allocations != 1 || throwing_allocations
        || constructions || destructions) return 1;
    union Storage { long long alignment; unsigned char data[2 * sizeof(Object)]; } storage;
    Tag tag = {3};
    Object* objects = new (tag, storage.data) Object[2];
    if ((void*)objects == (void*)storage.data || objects->payload != 7
        || placements != 1 || placement_tags != 3
        || placement_bytes < 2 * (int)sizeof(Object)
        || constructions != 2 || destructions) return 2;
    objects[0].~Object(); objects[1].~Object();
    return destructions != 2;
}

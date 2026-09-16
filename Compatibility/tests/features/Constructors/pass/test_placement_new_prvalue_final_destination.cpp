// EXPECT_EXIT: 0
// EXPECT_COMPILE_ARGS: -Werror
#include <new>
static int constructed, destroyed;
struct Object {
    Object *self;
    int value;
    explicit Object(int number) : self(this), value(number) { ++constructed; }
    Object(const Object&) = delete;
    Object(Object&&) = delete;
    ~Object() { ++destroyed; }
};
Object make(int number) { return Object(number); }
union Storage { Object *alignment; unsigned char bytes[sizeof(Object)]; };
int main() {
    Storage first, second, third;
    Object *a = new(first.bytes) Object(Object(3));
    Object *b = new(second.bytes) Object(make(5));
    bool choose_first = true;
    Object *c = new(third.bytes) Object(choose_first ? Object(7) : Object(9));
    if (a->self != a || b->self != b || c->self != c) return 1;
    if (a->value != 3 || b->value != 5 || c->value != 7) return 2;
    if (constructed != 3 || destroyed) return 3;
    c->~Object(); b->~Object(); a->~Object();
    return destroyed != 3;
}

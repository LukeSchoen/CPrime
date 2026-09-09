// EXPECT_EXIT: 0
int alive;
extern "C" void abort();
struct Audit { ~Audit() { if (alive) abort(); } } audit;
struct Object {
    int value;
    Object() : value(37) { ++alive; }
    Object(const Object& other) : value(other.value) { ++alive; }
    ~Object() { --alive; }
};
Object make() { Object local; return local; }
const Object& global = make();
Object&& rvalue = make();
const Object& alias = global;
const Object& local_static() { static const Object& value = make(); return value; }
int main() {
    if (alive != 2 || global.value != 37 || rvalue.value != 37
        || &alias != &global) return 1;
    const Object& first = local_static();
    if (alive != 3 || first.value != 37) return 2;
    return &first != &local_static() || alive != 3;
}

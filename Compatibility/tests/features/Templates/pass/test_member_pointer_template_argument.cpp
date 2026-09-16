// EXPECT_EXIT: 0
struct Object { int value; int get() const { return value + 2; } };
template<int Object::*Member> int read(Object& object) { return object.*Member; }
template<int (Object::*Method)() const> int call(Object& object) { return (object.*Method)(); }
template<int Object::*Member> struct Tag {};
int identify(Tag<&Object::value>) { return 7; }
int main() {
    Object object;
    object.value = 13;
    if (read<&Object::value>(object) != 13) return 1;
    if (call<&Object::get>(object) != 15) return 2;
    return identify(Tag<&Object::value>()) != 7;
}

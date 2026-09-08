// EXPECT_EXIT: 0
struct Padding { virtual ~Padding() {} int padding; };
struct Face { virtual Face* self() = 0; virtual ~Face() {} int value; };
struct Object : Padding, Face {
    Object() { padding = 12; value = 31; }
    Object* self() { return this; }
};
int main() {
    Object object; Face* face = &object;
    Face* returned = face->self();
    return returned != face || returned->value != 31 || object.padding != 12;
}

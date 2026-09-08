// EXPECT_EXIT: 0
struct Face { virtual Face* self() = 0; virtual ~Face() {} };
struct Object : Face { Object* self() { return this; } };
int main() {
    Object object; Face* face = &object;
    return face->self() != face || dynamic_cast<Object*>(face->self()) != &object;
}

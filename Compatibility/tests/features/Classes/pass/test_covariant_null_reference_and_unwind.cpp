struct Padding { virtual ~Padding() {} int padding; };
struct Face {
    virtual Face* pointer(int mode) = 0;
    virtual Face& reference() = 0;
    virtual ~Face() {}
};
struct Object : Padding, Face {
    Object* pointer(int mode) {
        if (mode == 2) throw 17;
        return mode ? 0 : this;
    }
    Object& reference() { return *this; }
};
int main() {
    Object object;
    Face* face = &object;
    if (face->pointer(0) != face || face->pointer(1)) return 1;
    if (object.pointer(0) != &object || &object.reference() != &object) return 2;
    if (&face->reference() != face) return 3;
    try { face->pointer(2); return 4; }
    catch (int value) { return value != 17; }
}

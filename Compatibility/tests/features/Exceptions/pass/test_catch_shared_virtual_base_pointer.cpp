// EXPECT_EXIT: 0
struct Base { int value; };
struct Hidden : private virtual Base {};
struct Public : virtual Base {};
struct Other : virtual Base {};
struct Object : Hidden, Public, private Other {};
int check(Object* object) {
    try { throw object; }
    catch (Base* base) { if (base != static_cast<Base*>(object)) return 1; }
    catch (...) { return 2; }
    try { throw object; }
    catch (Other*) { return 3; }
    catch (...) {}
    return 0;
}
struct Left : Base {};
struct Right : Base {};
struct Ambiguous : Left, Right {};
int main() {
    Object object;
    if (check(&object) || check(0)) return 1;
    Ambiguous ambiguous;
    try { throw &ambiguous; }
    catch (Base*) { return 2; }
    catch (...) {}
    return 0;
}

struct Root { int value; };
struct Left : virtual Root { int read() { return value; } };
struct Right : virtual Root { void write(int v) { value = v; } };
struct Object : Left, Right {};
int main() {
    Object object;
    Left* left = &object;
    Right* right = &object;
    Root* a = left;
    Root* b = right;
    if (a != b) return 1;
    right->write(29);
    if (left->read() != 29 || left->value != 29 || right->value != 29) return 2;
    left = 0;
    return static_cast<Root*>(left) != 0;
}

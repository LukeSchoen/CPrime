// EXPECT_EXIT: 0
struct Object {
    template<class T> int get(T value) { return value + 3; }
    template<class T> int get(T left, T right) { return left + right; }
    operator int() { return 17; }
    int operator=(int value) { return value + 1; }
    int operator=(Object const&) { return 23; }
};
int main() {
    Object object;
    int (Object::*single)(int) = &Object::get;
    int (Object::*pair)(int, int) = &Object::get<int>;
    int (Object::*convert)() = &Object::operator int;
    int (Object::*assign)(int) = &Object::operator=;
    return (object.*single)(4) != 7 || (object.*pair)(3, 8) != 11
        || (object.*convert)() != 17 || (object.*assign)(5) != 6;
}

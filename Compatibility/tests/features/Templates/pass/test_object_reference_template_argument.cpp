// EXPECT_EXIT: 0
struct Object { int value; int operator()(int n) const { return value + n; } };
Object object;
template<Object &R> struct Reference {
    static int read() { return R(3); }
    static void write(int n) { R.value = n; }
};
template<Object *P> struct Pointer { static int read() { return P->value; } };
int main() {
    Reference<object>::write(19);
    return object.value != 19 || Reference<object>::read() != 22
           || Pointer<&object>::read() != 19;
}

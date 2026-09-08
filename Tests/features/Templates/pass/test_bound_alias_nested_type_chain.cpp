// EXPECT_EXIT: 0
struct Base { typedef int type; enum Enum { enumerator = 41 }; };
struct Outer {
    struct Nested : Base {
        typedef Nested self;
        static int value;
    };
    int Nested;
};
int Outer::Nested::value = 23;
template<class T> int read() {
    typedef typename T::Nested::type type;
    type value = T::Nested::value;
    return value;
}
template<class T> struct Derived : T::Nested::self {
    using typename T::Nested::type;
    using typename T::Nested::Enum;
    type value;
};
int main() { Derived<Outer> d; d.value = 29; return read<Outer>() != 23 || d.value != 29; }

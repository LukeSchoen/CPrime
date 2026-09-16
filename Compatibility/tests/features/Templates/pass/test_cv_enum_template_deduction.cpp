// EXPECT_COMPILE_ARGS: -Werror
#include <stddef.h>
enum Color { red = 7, blue = 9 };
typedef Color ColorAlias;
template<class T> struct Const { static int value() { return 0; } };
template<class T> struct Const<const T> { static int value() { return 1; } };
template<class T> void copy(T* target, const T* source) { *target = *source; }
template<class T> void nested_copy(T** target, const T** source) { **target = **source; }
template<class T> int pointed_const(T*) { return Const<T>::value(); }
template<class T> int referred_const(T&) { return Const<T>::value(); }
template<class T> int value_const(T) { return Const<T>::value(); }
template<class T> int qualified_reference(const T&) { return Const<T>::value(); }
int main() {
    Color a = red, b = blue;
    ColorAlias* target = &b;
    const Color* source = &a;
    copy(target, source);
    if (b != red) return 1;
    a = blue;
    nested_copy(&target, &source);
    if (b != blue) return 2;
    wchar_t x = L'a', y = L'b';
    const wchar_t* wide = &x;
    copy(&y, wide);
    if (y != L'a') return 3;
    const Color color = red;
    if (pointed_const(&color) != 1 || referred_const(color) != 1) return 4;
    if (value_const(color) != 0 || qualified_reference(color) != 0) return 5;
    const int integer = 3;
    if (pointed_const(&integer) != 1 || referred_const(integer) != 1) return 6;
    if (value_const(integer) != 0 || qualified_reference(integer) != 0) return 7;
    return 0;
}

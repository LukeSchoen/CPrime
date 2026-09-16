// EXPECT_COMPILE_ARGS: -Werror
#include <stddef.h>
template<class T> struct Kind { static int value() { return 0; } };
template<> struct Kind<wchar_t> { static int value() { return 1; } };
template<> struct Kind<unsigned short> { static int value() { return 2; } };
template<class T> int character_kind(T) { return Kind<T>::value(); }
template<class T> int string_kind(const T*) { return Kind<T>::value(); }
int main() {
    if (character_kind(L'x') != 1) return 1;
    if (character_kind((unsigned short) 'x') != 2) return 2;
    if (string_kind(L"text") != 1) return 3;
    const wchar_t* text = L"wide";
    return string_kind(text) != 1;
}

template<class T> struct Wrapper { T* value; };
template<class T> struct Unwrap { typedef T type; };
template<class T> struct Unwrap<Wrapper<T>> { typedef T& type; };
template<class A, class B> struct Same { static const bool value = false; };
template<class A> struct Same<A, A> { static const bool value = true; };

int main() {
    static_assert(Same<Unwrap<Wrapper<int>>::type, int&>::value, "nested pattern");
    static_assert(Same<Unwrap<Wrapper<const int>>::type, const int&>::value,
                  "nested const argument");
    static_assert(Same<Unwrap<long>::type, long>::value, "primary fallback");
    return 0;
}

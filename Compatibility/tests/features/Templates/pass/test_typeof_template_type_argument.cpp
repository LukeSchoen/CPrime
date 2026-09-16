// EXPECT_EXIT: 0
template<class T> struct Size { static const int value = sizeof(T); };
template<class T = __typeof__(1 == 1)> struct Default : Size<T> {};
int main() {
    return Default<>::value != sizeof(bool)
        || Size<__typeof__(1.0)>::value != sizeof(double);
}

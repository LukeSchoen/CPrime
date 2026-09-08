// EXPECT_EXIT: 0
template<class T> T make();
template<class T, int = sizeof(make<T>())> char sized(int);
template<class T> long sized(...);
typedef int Function();
template<class T, int = sizeof(T)> char type_sized(int);
template<class T> long type_sized(...);
int main() {
    return sizeof(sized<int>(0)) != 1
        || sizeof(sized<void>(0)) != sizeof(long)
        || sizeof(type_sized<Function>(0)) != sizeof(long);
}

// EXPECT_EXIT: 0
#include <utility>
template<bool B> struct Flag { int value() { return B ? 1 : 0; } };
template<class T> int signed_value() {
    Flag<std::is_signed<T>::value> flag;
    return flag.value();
}
struct Object {};
int main() {
    return signed_value<int>() == 1 && signed_value<unsigned int>() == 0
        && signed_value<const int>() == 1 && signed_value<float>() == 1
        && signed_value<Object>() == 0 && signed_value<int *>() == 0 ? 0 : 1;
}

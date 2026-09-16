// EXPECT_EXIT: 0
#include <type_traits>
#include <utility>

namespace probe {
template<class T> struct Identity { typedef T type; };
template<class T> typename Identity<T>::type read(T value) { return value; }
template<class T, class = void> struct Valid : std::false_type {};
template<class T> struct Valid<T, decltype((void)std::declval<T>().get())>
    : std::true_type {};
struct Object { int value; int get() const { return value; } };
}

int main() {
    enum { read = 41, Identity = 43 };
    int value = 7;
    if (probe::read(value) != 7 || read != 41 || Identity != 43) return 1;
    {
        typedef long long Identity;
        enum { read = 47 };
        Identity wide = 13;
        if (probe::read(wide) != 13 || read != 47) return 2;
        {
            struct Identity { int value; };
            Identity local = {19};
            probe::Object object = {local.value};
            if (probe::read(object).get() != 19 || !probe::Valid<probe::Object>::value)
                return 3;
            if (probe::Valid<Identity>::value || local.value != 19) return 4;
        }
        if (wide != 13 || read != 47) return 5;
    }
    { int read = 53; if (probe::read(read) != 53) return 6; }
    { short read = 59; if (probe::read(read) != 59) return 7; }
    return read != 41 || Identity != 43 || probe::read(value) != 7;
}

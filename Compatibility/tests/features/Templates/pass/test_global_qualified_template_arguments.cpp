// EXPECT_COMPILE_ARGS: -Werror
struct Item { int words[3]; };
typedef int Number;
constexpr int Count = 7;
template<class T> struct Wrap { typedef T type; };
template<class T> struct Keep { typedef T type; };
template<class T> int type_size() { return sizeof(T); }
template<int N> struct Constant { static int value() { return N; } };
namespace Values { struct Item { int words[5]; }; }

namespace Nested {
struct Item { char value; };
typedef char Number;
template<class T> struct Wrap { typedef char type; };
template<class T> struct Use {
    typedef typename ::Wrap<T>::type direct_element;
    typedef typename ::Wrap<::Item>::type direct_global_element;
    typedef typename Keep<::Wrap<T>>::type global_wrap;
    typedef typename Keep<::Item>::type global_element;
    static int size() {
        if (sizeof(direct_element) != sizeof(T)) return -1;
        if (sizeof(direct_global_element) != sizeof(global_element)) return -2;
        return sizeof(typename global_wrap::type) + sizeof(global_element);
    }
};
int check() {
    if (type_size<::Item>() != 3 * sizeof(int)) return 1;
    if (type_size<::Number>() != sizeof(int)) return 2;
    if (type_size<::Values::Item>() != 5 * sizeof(int)) return 3;
    if (type_size<const ::Item*>() != sizeof(void*)) return 4;
    if (Use<Item>::size() != 1 + 3 * sizeof(int)) return 5;
    return 0;
}
}
int main() {
    typedef char Number;
    constexpr int Count = 2;
    if (type_size<::Number>() != sizeof(int)) return 6;
    if (Constant<::Count + 1>::value() != 8) return 7;
    return Nested::check();
}

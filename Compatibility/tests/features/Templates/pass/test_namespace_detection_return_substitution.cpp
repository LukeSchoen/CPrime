// EXPECT_EXIT: 0
#include <type_traits>
#include <utility>

namespace containers {
template<class T, class = int> struct has_resize : std::false_type {};
template<class T>
struct has_resize<T, decltype((void)std::declval<T>().resize(1), 0)>
    : std::true_type {};

template<class C>
inline typename std::enable_if<has_resize<C>::value, void>::type
resize(C& value, int size) { value.resize(size); }
template<class C>
inline typename std::enable_if<!has_resize<C>::value, void>::type
resize(C& value, int size) { if (value.size() != size) throw 1; }

struct Dynamic {
    int count;
    void resize(int size) { count = size; }
    int size() const { return count; }
};
struct Fixed { int size() const { return 4; } };
template<class C> struct User {
    C value;
    void change(int size) { resize(value, size); }
};

template<class C>
typename std::enable_if<has_resize<C>::value, const C>::type&
identity(const C& value) { return value; }
}

int main() {
    containers::User<containers::Dynamic> dynamic;
    containers::User<containers::Fixed> fixed;
    dynamic.change(7);
    fixed.change(4);
    if (dynamic.value.count != 7) return 1;
    dynamic.change(9);
    fixed.change(4);
    if (&containers::identity(dynamic.value) != &dynamic.value) return 2;
    try { fixed.change(3); return 3; } catch (int value) {
        if (value != 1) return 4;
    }
    return dynamic.value.count != 9;
}

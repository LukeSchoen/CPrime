// EXPECT_EXIT: 0
#include <type_traits>
#include <utility>

namespace sample {
template<class T> struct Traits { typedef T type; };
template<class T> typename Traits<T>::type identity(T value) { return value; }

template<class T, class = int> struct has_resize : std::false_type {};
template<class T>
struct has_resize<T, decltype((void)std::declval<T>().resize(1), 0)>
    : std::true_type {};

template<class T>
typename std::enable_if<has_resize<T>::value, int>::type
change(T& value, int count) { value.resize(count); return value.size(); }
template<class T>
typename std::enable_if<!has_resize<T>::value, int>::type
change(T& value, int count) { return value.size() + count; }

struct Dynamic {
    int count;
    void resize(int value) { count = value; }
    int size() const { return count; }
};
struct Fixed { int size() const { return 3; } };
}

int main() {
    int Traits = 9;
    if (sample::identity(7) + Traits != 16) return 1;
    {
        typedef double Traits;
        Traits value = 2.5;
        if (sample::identity(11) != 11 || value != 2.5) return 2;
    }
    {
        struct Traits { int value; };
        struct Traits local = {23};
        if (sample::identity(local).value != 23) return 3;
        if (sample::identity(13) != 13 || local.value != 23) return 4;
    }

    int has_resize = 17;
    sample::Fixed fixed;
    sample::Dynamic dynamic = {0};
    // Alternate successful and failed substitution inside one live caller
    // expression, then verify that the caller's bindings still resolve.
    for (int i = 0; i < 3; ++i) {
        int before = Traits;
        int total = before + sample::change(fixed, i)
                           + sample::change(dynamic, i + 4);
        if (total != before + 7 + 2 * i) return 5;
        if (Traits != 9 || has_resize != 17 || before != 9) return 6;
    }
    return dynamic.count != 6;
}

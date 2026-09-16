#include <vector>
template<class T> struct Container {
    std::vector<T> values;
    using Size = typename decltype(values)::size_type;
    struct Range { Size first, last; };
    Size size() { return values.size(); }
};
int main() {
    Container<int> integers;
    Container<double> reals;
    Container<int>::Range range;
    range.first = 2;
    range.last = 5;
    return integers.size() != 0 || reals.size() != 0
        || range.last - range.first != 3;
}

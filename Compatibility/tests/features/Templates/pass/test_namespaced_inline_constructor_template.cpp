// EXPECT_EXIT: 0
namespace detail {
inline int copy_value(int value) { return value; }
struct Holder {
    int value;
    template<class T> explicit Holder(T input) : value(detail::copy_value(input.value)) {}
};
}
struct Input { int value; };
int main() {
    Input input = {17};
    detail::Holder holder(input);
    return holder.value == 17 ? 0 : 1;
}

// EXPECT_EXIT: 0
struct Item { int value; };
namespace library {
template<class T> struct Holder { T value; };
template<class T> T copy(T value) { return value; }
template<class T> Holder<T> wrap(T value) {
    Holder<T> holder;
    holder.value = copy(value);
    return holder;
}
}
int main() {
    Item item = {23};
    library::Holder<Item> first = library::wrap(item);
    library::Holder<Item> second = library::wrap(item);
    return first.value.value == 23 && second.value.value == 23 ? 0 : 1;
}

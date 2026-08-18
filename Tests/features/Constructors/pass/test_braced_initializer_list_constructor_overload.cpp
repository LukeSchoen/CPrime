#include <initializer_list>

struct Item
{
    Item(int value) : value(value) {}
    int value;
};

template<typename T>
struct List
{
    List(long long size) : selected(1) {}
    List(const std::initializer_list<T> &values) : selected(2) {}

    int selected;
};

int main()
{
    List<Item> values = { Item(1), Item(2), Item(3) };
    return values.selected == 2 ? 0 : 1;
}

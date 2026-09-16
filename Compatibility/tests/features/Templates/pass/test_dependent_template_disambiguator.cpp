// EXPECT_EXIT: 0
template<template<class> class T> struct Apply { T<int> value; };
struct Types { template<class T> struct Item { T value; }; };
template<class T> struct Consumer {
    Apply<T::template Item> applied;
    typename T::template Item<int> item;
};
int main() {
    Consumer<Types> c;
    c.applied.value.value = 13;
    c.item.value = 17;
    return c.applied.value.value + c.item.value != 30;
}

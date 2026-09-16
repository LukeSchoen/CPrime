// EXPECT_EXIT: 0
template<class T> struct Cell { T value; int read() { return value; } };
template<template<class> class C, class T> struct Owner {
    C<T> cell;
    int read() { return cell.read(); }
};
template<template<class> class C> Owner<C, int> make() {
    Owner<C, int> result;
    result.cell.value = 37;
    return result;
}
template<class T, class U = T> struct Pair { T first; U second; };
template<template<class, class> class C> struct Two { C<int, int> pair; };
int main() {
    Owner<Cell, int> owner = make<Cell>();
    Two<Pair> two;
    two.pair.first = 3;
    two.pair.second = 5;
    return owner.read() != 37 || two.pair.first + two.pair.second != 8;
}

template<class> struct Cell;
template<template<class> class, class, class> struct Select;
template<template<class> class C> struct Select<C, C<int>, int> { enum { value = 7 }; };
template<template<class> class C, class T> struct Select<C, C<int>, T>;
template<template<class> class C, class T> struct Match;
template<template<class> class C, class T> struct Match<C, C<T> > { enum { value = 11 }; };
int main() { return Select<Cell, Cell<int>, int>::value != 7
                 || Match<Cell, Cell<double> >::value != 11; }

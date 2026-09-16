// EXPECT_EXIT: 0
#include <typeinfo>
template<class T> struct Invalid { typename T::missing field; };
template<class T> struct Unused { int value; };
template<class T> struct Valid { T value; };
int select(Invalid<int>*);
int select(...) { return 17; }
int read(Valid<int> *p) { return p->value; }
struct Copy {
    int value;
    Copy() : value(23) {}
    Copy(Copy const& other) : value(other.value) {}
    Copy(Invalid<int> const&);
};
int main() {
    int *pointer = 0;
    Copy original, copied(original);
    Valid<int> valid = {11};
    Unused<Invalid<void> > unused = {7};
    return select(pointer) != 17 || copied.value != 23 || read(&valid) != 11
        || unused.value != 7 || typeid(Invalid<void>*) != typeid(Invalid<void>*);
}

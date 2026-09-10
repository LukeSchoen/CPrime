// A conversion function of a class template that is declared in the class and
// defined out of class used to register two conversion functions for the
// instantiation: the class-pattern replay spells the conversion target through
// a bound-type alias, while the out-of-class definition replay spells the
// substituted type.  The call site then saw two equally ranked conversions and
// reported `cannot convert`, and the definition emitted its body under a
// separate mangled name.  Both spellings must bind to the one declaration.
struct element { int value; };

template<class T> struct holder
{
    T *storage;
    holder() : storage(0) { }
    operator T *() const;
    operator T() const;
};

template<class T> holder<T>::operator T *() const { return storage; }
template<class T> holder<T>::operator T() const { return *storage; }

int main()
{
    element e;
    e.value = 42;
    holder<element> h;
    h.storage = &e;
    element *p = h;
    if (p != &e)
        return 1;
    element copy = h;
    if (copy.value != 42)
        return 2;
    if ((element *) h != &e)
        return 3;
    return 0;
}

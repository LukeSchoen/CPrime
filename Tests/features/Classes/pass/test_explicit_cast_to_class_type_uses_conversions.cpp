// An explicit cast to a class type only looked for a converting constructor.
// It must also use a conversion function of the source class
// (`(element) p` with `plain::operator element()`) and slice a derived object
// to its base subobject (`(base) Derived()`); both shapes used to report
// `no matching constructor for cast to '...'`.
struct element { int value; };

struct plain
{
    element *storage;
    operator element *() const;
    operator element() const;
};

plain::operator element *() const { return storage; }
plain::operator element() const { return *storage; }

struct base { int a; base() : a(1) { } };
struct derived : base { int b; derived() { a = 2; b = 3; } };

int main()
{
    element e;
    e.value = 7;
    plain p;
    p.storage = &e;
    element converted = (element) p;
    if (converted.value != 7)
        return 1;
    if ((element *) p != &e)
        return 2;
    base sliced = (base) derived();
    if (sliced.a != 2)
        return 3;
    return 0;
}

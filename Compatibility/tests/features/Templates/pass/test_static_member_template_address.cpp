// Taking the address of a static member function template written with an
// explicit argument list names one function even though the member is declared
// inside its class and defined out of line: the declaration and the definition
// are two member records that instantiate the same specialization.  The
// address is also formed in a class-template member-initializer and the
// resulting callback is called through the pointer.

namespace dx
{
    int once_hits;

    struct onc
    {
        typedef void (*Cb)();

        onc(Cb cb) : callback(cb) {}

        Cb callback;
    };

    struct grac
    {
        template<class Derived> static void once();
    };

    template<class Derived>
    struct tonc : onc
    {
        tonc() : onc(&grac::once<Derived>) {}

        void fire() { callback(); }
    };

    template<class Derived> void grac::once() { once_hits = once_hits + 1; }
}

int setR_selected;

struct Nosm
{
    int m_R;
};

namespace
{
    template<typename T, int = sizeof(&T::m_R)>
    struct has_R { };

    template<typename T>
    inline void setR(T* m, has_R<T>* = 0) { (void)m; setR_selected = 1; }

    inline void setR(...) { setR_selected = 2; }
}

template<typename M>
struct Qmi
    : dx::tonc<Qmi<M> >
{
    void h()
    {
        setR(&msg);
    }

    M msg;
};

int main()
{
    Qmi<Nosm> x;
    x.h();
    x.fire();
    if (setR_selected != 1) return 1;
    if (dx::once_hits != 1) return 2;
    return 0;
}

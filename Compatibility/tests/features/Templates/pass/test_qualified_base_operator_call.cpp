// EXPECT_EXIT: 0
// A member function may call a base class template's operator through a
// qualified name: `B<T>::operator=(rhs)`.
template<class T> struct B
{
    int value;
    B& operator=(const B& rhs) { value = rhs.value; return *this; }
};

template<class T> struct D : B<T>
{
    D& operator=(const D& rhs)
    {
        B<T>::operator=(rhs);
        return *this;
    }
};

int main()
{
    D<int> a, b;
    a.value = 7;
    b = a;
    return b.value != 7;
}

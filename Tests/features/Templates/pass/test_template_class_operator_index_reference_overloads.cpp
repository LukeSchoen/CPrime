typedef long long i64;

template<typename T>
struct RefList {
    T value;

    template<i64 N>
    RefList(const T(&elements)[N]);

    template<typename U>
    explicit RefList(const RefList<U> other)
      : value(0)
    {
        (void)other;
    }

    RefList()
      : value(0)
    {
    }

    const T& operator[](int index) const;
    T& operator[](int index);
};

template<typename T>
const T& RefList<T>::operator[](int index) const
{
    (void)index;
    return value;
}

template<typename T>
T& RefList<T>::operator[](int index)
{
    (void)index;
    return value;
}

int main()
{
    RefList<int> list;
    list.value = 9;
    if (list[0] != 9)
        return 1;
    return 0;
}

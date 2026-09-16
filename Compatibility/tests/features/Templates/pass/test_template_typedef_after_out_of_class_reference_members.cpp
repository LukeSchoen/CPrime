typedef long long i64;
typedef unsigned char ui8;

template<typename T>
struct AliasList {
    T value;

    AliasList();
    const T& operator[](i64 index) const;
    T& operator[](i64 index);
    AliasList<T>& operator=(const AliasList<T> &rhs);
    const T& At(i64 index) const;
    T& At(i64 index);
    i64 Size() const;
};

template<typename T> AliasList<T>::AliasList()
  : value(0)
{
}

template<typename T> AliasList<T>& AliasList<T>::operator=(const AliasList<T> &rhs)
{
    value = rhs.value;
    return *this;
}

template<typename T> const T& AliasList<T>::At(i64 index) const
{
    (void)index;
    return value;
}

template<typename T> T& AliasList<T>::At(i64 index)
{
    (void)index;
    return value;
}

template<typename T> const T& AliasList<T>::operator[](i64 index) const
{
    (void)index;
    return value;
}

template<typename T> T& AliasList<T>::operator[](i64 index)
{
    (void)index;
    return value;
}

template<typename T> i64 AliasList<T>::Size() const
{
    return 1;
}

typedef AliasList<ui8> ByteList;

int main()
{
    ByteList list;
    list.value = 3;
    if (list[0] != 3)
        return 1;
    return list.Size() == 1 ? 0 : 2;
}

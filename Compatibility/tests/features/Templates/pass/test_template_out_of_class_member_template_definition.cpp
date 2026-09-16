typedef long long i64;

template<typename T>
struct StaticCopyList {
    T value;

    StaticCopyList()
      : value(0)
    {
    }

    template<i64 N>
    StaticCopyList(const T(&elements)[N]);
};

template<typename T>
template<i64 N>
StaticCopyList<T>::StaticCopyList(const T(&elements)[N])
  : value(elements[0])
{
}

typedef StaticCopyList<int> IntList;

int main()
{
    int values[1];
    values[0] = 7;
    IntList list(values);
    if (list.value != 7)
        return 1;
    return 0;
}

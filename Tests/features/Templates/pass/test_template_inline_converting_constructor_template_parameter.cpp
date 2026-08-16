template<typename T>
struct InlineConvertingCtorList {
    T value;

    InlineConvertingCtorList()
      : value(0)
    {
    }

    template<typename U>
    explicit InlineConvertingCtorList(const InlineConvertingCtorList<U> other)
      : value(1)
    {
        for (const U &o : other)
            value = T(o);
    }

    const T *begin() const { return 0; }
    const T *end() const { return 0; }
};

int main()
{
    InlineConvertingCtorList<int> list;
    return list.value;
}

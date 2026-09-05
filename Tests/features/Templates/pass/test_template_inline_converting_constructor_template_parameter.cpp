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

    const T *begin() const { return &value; }
    const T *end() const { return &value + 1; }
    int Size() const { return 1; }
    const T &operator[](int index) const { return value; }
};

int main()
{
    InlineConvertingCtorList<int> source;
    source.value = 7;
    InlineConvertingCtorList<long> converted(source);
    return converted.value == 7 ? 0 : 1;
}

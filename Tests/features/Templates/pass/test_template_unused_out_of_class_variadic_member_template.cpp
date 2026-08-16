template<typename T>
struct VariadicMemberBox {
    T value;

    VariadicMemberBox()
      : value(3)
    {
    }

    template<typename... Args>
    void set(Args... args);
};

template<typename T>
template<typename... Args>
void VariadicMemberBox<T>::set(Args... args)
{
    value = 0;
}

int main()
{
    VariadicMemberBox<int> box;
    return box.value != 3;
}

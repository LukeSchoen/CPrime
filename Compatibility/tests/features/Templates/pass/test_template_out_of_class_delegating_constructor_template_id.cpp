template<typename T>
struct DelegatingTemplateCtor {
    T value;

    DelegatingTemplateCtor();
    DelegatingTemplateCtor(int value);
};

template<typename T>
DelegatingTemplateCtor<T>::DelegatingTemplateCtor()
  : value(0)
{
}

template<typename T>
DelegatingTemplateCtor<T>::DelegatingTemplateCtor(int other)
  : DelegatingTemplateCtor<T>()
{
    value = other;
}

int main()
{
    DelegatingTemplateCtor<int> item;
    (void)item;
    return 0;
}

template<typename T>
int size_value();

template<typename T>
int size_value()
{
    return sizeof(T);
}

int main()
{
    return size_value(int)() == sizeof(int) ? 0 : 1;
}

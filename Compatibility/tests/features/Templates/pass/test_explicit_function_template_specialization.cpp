template<typename T>
int bits();

template<typename T>
int bits()
{
    return sizeof(T) * 8;
}

template<>
int bits<char>()
{
    return 8;
}

int main()
{
    return bits(int)() + bits(char)() - 40;
}

template<typename T>
inline constexpr int value_bits();

template<typename T>
inline constexpr int value_bits()
{
    return sizeof(T) * 8;
}

template<>
inline constexpr int value_bits<char>()
{
    return 8;
}

int main()
{
    return value_bits(int)() + value_bits(char)() - 40;
}

template<typename T, typename... Args>
T sum(T first, Args... args)
{
    return first + sum(args...);
}

template<typename T>
T sum(T last)
{
    return last;
}

int main()
{
    return sum(1, 2, 3) - 6;
}

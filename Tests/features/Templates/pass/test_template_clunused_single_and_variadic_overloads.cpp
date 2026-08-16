template<typename T>
void clUnused(const T &val)
{
    (void)val;
}

template<typename T, typename... Args>
void clUnused(const T first, Args... args)
{
    (void)first;
    clUnused(args...);
}

int main()
{
    int a = 1;
    int b = 2;
    clUnused(a);
    clUnused(a, b);
    return 0;
}

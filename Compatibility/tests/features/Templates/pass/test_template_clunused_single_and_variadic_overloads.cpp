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

void check(const bool& flag, const int& line, const char* text) { clUnused(text, text, text, flag, line); }

int main()
{
    int a = 1;
    int b = 2;
    clUnused(a);
    clUnused(0);
    clUnused(a, b);
    const char* text = "text";
    clUnused(text);
    clUnused(text, a, text);
    check(true, a, text);
    return 0;
}

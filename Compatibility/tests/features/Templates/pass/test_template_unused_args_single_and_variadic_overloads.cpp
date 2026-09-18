// EXPECT_EXIT: 0
// A single-argument and a variadic overload of one template name, both called
// with argument lists the variadic one has to accept.
template<typename T>
void unusedArg(const T &val)
{
    (void)val;
}

template<typename T, typename... Args>
void unusedArg(const T first, Args... args)
{
    (void)first;
    unusedArg(args...);
}

void check(const bool& flag, const int& line, const char* text) { unusedArg(text, text, text, flag, line); }

int main()
{
    int a = 1;
    int b = 2;
    unusedArg(a);
    unusedArg(0);
    unusedArg(a, b);
    const char* text = "text";
    unusedArg(text);
    unusedArg(text, a, text);
    check(true, a, text);
    return 0;
}

template<typename T>
void unused(const T &value) { (void)value; }

template<typename T, typename... Args>
void unused(const T first, Args... args) { (void)first; unused(args...); }

int main()
{
    bool value = true;
    unused(value);
    return 0;
}

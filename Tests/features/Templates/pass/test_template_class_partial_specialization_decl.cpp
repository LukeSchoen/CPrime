template<typename T, bool Flag = false>
struct Helper {
    static int value() { return 1; }
};

template<typename T>
struct Helper<T, true> {
    static int value() { return 2; }
};

int main()
{
    return 0;
}

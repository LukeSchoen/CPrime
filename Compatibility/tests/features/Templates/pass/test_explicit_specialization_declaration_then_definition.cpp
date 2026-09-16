// EXPECT_EXIT: 0
// An explicit specialization declaration may be completed by a later
// definition of the same specialization; a second definition is still an
// error, but declaration-then-definition must reuse the instance.
template<class T> struct ct
{
    static const int value = 1;
};

template<> struct ct<char>;
template<> struct ct<char>
{
    static const int value = 2;
};

int main()
{
    return ct<int>::value != 1 || ct<char>::value != 2;
}

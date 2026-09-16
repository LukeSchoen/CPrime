#define DEFAULT_VALUE 37

int value_or_default(int value = DEFAULT_VALUE);

#undef DEFAULT_VALUE

int value_or_default(int value)
{
    return value;
}

int main()
{
    return value_or_default() == 37 ? 0 : 1;
}

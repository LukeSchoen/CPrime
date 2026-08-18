template<typename T>
void copy_one(T *target, T *source, int count = 1)
{
    for (int i = 0; i < count; ++i)
        target[i] = source[i];
}

int main()
{
    int source = 42;
    int target = 0;
    copy_one(&target, &source);
    return target == 42 ? 0 : 1;
}

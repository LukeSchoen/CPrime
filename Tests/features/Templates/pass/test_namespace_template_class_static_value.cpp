#include <utility>

template<typename T>
int use_trait()
{
    if (std::is_trivially_copyable<T>::value)
        return 1;
    return 0;
}

int main()
{
    return 0;
}

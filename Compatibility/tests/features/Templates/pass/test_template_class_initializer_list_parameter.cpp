#include <initializer_list>

template<typename T>
class InitListUser
{
public:
    InitListUser(const std::initializer_list<T> &values);
    InitListUser& operator=(const std::initializer_list<T> &values);
};

int main()
{
    return 0;
}

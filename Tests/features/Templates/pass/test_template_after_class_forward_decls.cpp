#include <initializer_list>

class ReadStream;
class WriteStream;

template<typename T>
class ForwardList
{
public:
    ForwardList();
    ForwardList(const std::initializer_list<T> &values);
};

int main()
{
    return 0;
}

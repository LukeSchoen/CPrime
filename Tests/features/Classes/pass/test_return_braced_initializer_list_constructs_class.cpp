#include <initializer_list>

template<typename T>
class ReturnList
{
public:
  ReturnList() : count(0) {}
  ReturnList(const std::initializer_list<T> &values) : count(values.size()) {}

  int count;
};

ReturnList<int> make_return_list(int a, int b, int c)
{
  return{ a, b, c };
}

int main()
{
  return 0;
}

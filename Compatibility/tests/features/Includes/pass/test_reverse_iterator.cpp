#include <iterator>

int main()
{
  int values[4] = { 1, 2, 3, 4 };
  std::reverse_iterator<int*> first(values + 4);
  std::reverse_iterator<int*> last(values);

  if (*first != 4)
    return 1;
  if (first[1] != 3)
    return 2;
  if (last - first != 4)
    return 3;
  if (*(first + 2) != 2)
    return 4;
  if (first.base() != values + 4)
    return 5;
  if (std::make_reverse_iterator(values + 4).base() != values + 4)
    return 6;
  return 0;
}

// std::istreambuf_iterator is the input counterpart of ostreambuf_iterator;
// boost::date_time's special_values_parser declares one as a member typedef.
#include <iterator>
#include <sstream>
#include <string>

int main()
{
  std::istringstream in(std::string("hello"));
  std::istreambuf_iterator<char> it(in);
  std::istreambuf_iterator<char> end;
  std::string text;
  while (it != end)
  {
    text += *it;
    ++it;
  }
  if (text != "hello")
    return 1;

  std::istringstream empty(std::string(""));
  std::istreambuf_iterator<char> empty_it(empty);
  if (empty_it != end)
    return 2;
  return 0;
}

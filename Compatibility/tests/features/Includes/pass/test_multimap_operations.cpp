// std::multimap is a standard container; boost::date_time's string_parse_tree
// declares a recursive multimap and needs every key kept in insertion order.
#include <map>
#include <string>

int main()
{
  std::multimap<int, std::string> values;
  values.insert(std::make_pair(1, std::string("a")));
  values.insert(std::make_pair(2, std::string("b")));
  values.insert(std::make_pair(1, std::string("c")));
  values.insert(std::make_pair(1, std::string("d")));
  if (values.size() != 4)
    return 1;
  if (values.count(1) != 3 || values.count(2) != 1 || values.count(3) != 0)
    return 2;

  std::string order;
  for (std::multimap<int, std::string>::iterator it = values.begin();
       it != values.end(); ++it)
    order += it->second;
  if (order != "acdb")
    return 3;

  if (values.lower_bound(1)->second != "a")
    return 4;
  if (values.upper_bound(1)->second != "b")
    return 5;
  int in_range = 0;
  for (std::multimap<int, std::string>::iterator it = values.equal_range(1).first;
       it != values.equal_range(1).second; ++it)
    ++in_range;
  if (in_range != 3)
    return 6;

  if (values.erase(1) != 3)
    return 7;
  if (values.size() != 1 || values.count(2) != 1)
    return 8;

  std::multimap<int, std::string> copy(values);
  if (copy.size() != 1 || copy.begin()->second != "b")
    return 9;
  copy.clear();
  if (!copy.empty())
    return 10;
  return 0;
}

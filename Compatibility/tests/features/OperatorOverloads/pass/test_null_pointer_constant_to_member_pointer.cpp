// A null pointer constant converts to a pointer-to-member parameter
// ([conv.ptr]/[conv.mem]).  boost::type_traits probes that with the
// member-pointer SFINAE shape below, so a rejection makes every class look
// like a non-class and breaks is_base_of for the whole standard library.
#include <stddef.h>

typedef char yes_type;
struct no_type { char data[8]; };

class Probe {};
struct Other {};
struct Derived : Probe {};

template <class U> yes_type method_tester(void (U::*)(void));
template <class U> no_type method_tester(...);
template <class U> yes_type data_tester(int U::*);
template <class U> no_type data_tester(...);

template <typename T>
struct is_class_probe
{
  static const bool value = sizeof(method_tester<T>(0)) == sizeof(yes_type);
};

template <typename T>
struct has_data_member_probe
{
  static const bool value = sizeof(data_tester<T>(0)) == sizeof(yes_type);
};

int main()
{
  if (!is_class_probe<Probe>::value)
    return 1;
  if (!is_class_probe<Derived>::value)
    return 2;
  if (!has_data_member_probe<Probe>::value)
    return 3;
  if (is_class_probe<int>::value)
    return 4;
  if (has_data_member_probe<int>::value)
    return 5;
  return 0;
}

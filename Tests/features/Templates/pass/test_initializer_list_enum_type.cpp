#include <initializer_list>

enum KeyType
{
  KeyTypeA,
  KeyTypeB
};

int main()
{
  std::initializer_list<KeyType> values = { KeyTypeA, KeyTypeB };
  (void)values;
  return 0;
}

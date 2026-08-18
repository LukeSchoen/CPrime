#include <initializer_list>

enum BraceKey
{
  BraceKeyA,
  BraceKeyB
};

int main()
{
  (void)std::initializer_list<BraceKey>{ BraceKeyA, BraceKeyB };
  return 0;
}

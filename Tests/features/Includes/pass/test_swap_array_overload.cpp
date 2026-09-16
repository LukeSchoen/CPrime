// C++11 declares swap for two arrays of the same length. The value overload
// cannot stand in for it: deducing T as the array type makes the temporary
// declaration inside the body ill-formed.

#include <utility>

struct Pair
{
  Pair() : a(0), b(0) {}
  Pair(int left, int right) : a(left), b(right) {}
  int a, b;
};

int main()
{
  int left[3] = {1, 2, 3};
  int right[3] = {4, 5, 6};
  std::swap(left, right);
  if (left[0] != 4 || left[1] != 5 || left[2] != 6) return 1;
  if (right[0] != 1 || right[1] != 2 || right[2] != 3) return 2;

  Pair one[2] = {Pair(1, 2), Pair(3, 4)};
  Pair two[2] = {Pair(5, 6), Pair(7, 8)};
  std::swap(one, two);
  if (one[0].a != 5 || one[1].b != 8) return 3;
  if (two[0].a != 1 || two[1].b != 4) return 4;

  int scalarA = 10, scalarB = 20;
  std::swap(scalarA, scalarB);
  if (scalarA != 20 || scalarB != 10) return 5;
  return 0;
}

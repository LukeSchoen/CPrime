#include <algorithm>
struct Forward {
  int *p;
  int &operator*() const { return *p; }
  Forward &operator++() { ++p; return *this; }
  bool operator!=(const Forward &other) const { return p != other.p; }
};
int main() {
  int a[] = {2,4,6,4};
  Forward first = {a}, last = {a+4};
  if (std::find(first,last,4).p != a+1) return 1;
  if (std::find(first,last,9).p != a+4) return 2;
  if (std::find(last,last,6).p != a+4) return 3;
  return std::find_if(first,last,[](int n) { return n > 4; }).p != a+2;
}

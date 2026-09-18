// C++17 gap probe: alt_operators and cxx17_and_or_keywords. The alternative
// operator tokens are keywords, so they must work without <iso646.h>.
int main() {
  int x = 1;
  if (not (x > 0 and x < 2)) return 1;
  if (x < 0 or x > 2) return 2;
  if ((x bitand 3) != 1) return 3;
  if ((x xor 1) != 0) return 4;
  if (compl 0 != ~0) return 5;
  return 0;
}

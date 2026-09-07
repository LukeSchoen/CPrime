// EXPECT_EXIT: 0
// The GCC parser-stack regression has 11,000 else-if branches.
#define ONE else if (++count == target) return count;
#define TEN ONE ONE ONE ONE ONE ONE ONE ONE ONE ONE
#define HUNDRED TEN TEN TEN TEN TEN TEN TEN TEN TEN TEN
#define THOUSAND HUNDRED HUNDRED HUNDRED HUNDRED HUNDRED HUNDRED HUNDRED HUNDRED HUNDRED HUNDRED
int select(int target) {
  int count = 0;
  if (target == 0) return 0;
  THOUSAND THOUSAND THOUSAND THOUSAND THOUSAND THOUSAND
  THOUSAND THOUSAND THOUSAND THOUSAND THOUSAND
  else return -1;
}
int main() {
  return select(0) || select(1) != 1 || select(5001) != 5001
    || select(11000) != 11000 || select(11001) != -1;
}

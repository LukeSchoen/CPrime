// EXPECT_EXIT: 0
#include <string.h>
#define JOIN(a, b) a ## b
#define QUOTE_RAW(a) #a
#define QUOTE(a) QUOTE_RAW(a)
int main() {
  if (strcmp(QUOTE(QUOTE(JOIN(\u00c1, \u00C1))), "\"\\u00c1\\u00C1\"")) return 1;
  if (strcmp(QUOTE(JOIN(prefix, \U000000c1)), "prefix\U000000c1")) return 2;
  return 0;
}

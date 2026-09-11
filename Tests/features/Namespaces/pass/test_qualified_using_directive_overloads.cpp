// EXPECT_EXIT: 0
/* A qualified call into a namespace that inherits one name through several
   using-directives selects against the whole imported overload set. */
namespace First {
  int pick(int) { return 1; }
}
namespace Second {
  int pick(char *) { return 2; }
}
namespace Combined {
  using namespace First;
  using namespace Second;
}

int main() {
  char value = 0;
  if (Combined::pick(3) != 1) return 1;
  if (Combined::pick(&value) != 2) return 2;
  int (*selected)(char *) = &Combined::pick;
  if (selected(&value) != 2) return 3;
  return 0;
}

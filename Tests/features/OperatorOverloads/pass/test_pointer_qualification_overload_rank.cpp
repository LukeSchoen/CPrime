// EXPECT_EXIT: 0
struct Select {
  int operator<<(const char*) { return 1; }
  int operator<<(const void*) { return 2; }
};
int select(const char*) { return 3; }
int select(const void*) { return 4; }
int mutable_select(char*) { return 5; }
int mutable_select(const char*) { return 6; }
int main() {
  Select object;
  char text[] = "x";
  const char fixed[] = "y";
  char *pointer = text;
  if ((object << "literal") != 1) return 1;
  if ((object << text) != 1) return 2;
  if ((object << pointer) != 1) return 3;
  if ((object << fixed) != 1) return 4;
  if (select(text) != 3 || select(pointer) != 3) return 5;
  if (mutable_select(text) != 5 || mutable_select(pointer) != 5) return 6;
  if (mutable_select(fixed) != 6) return 7;
  return 0;
}

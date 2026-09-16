// EXPECT_EXIT: 0
int calls;
bool next_value() { ++calls; return false; }
int main() {
  if (bool(next_value())) return 1;
  while (bool(next_value())) return 2;
  for (; bool(next_value());) return 3;
  if (int (value) = 4) { if (value != 4) return 4; }
  return calls != 3;
}

// EXPECT_EXIT: 0
int select(const char* const* value) { return **value == 'x' ? 1 : 0; }
int select(const void*) { return 2; }
struct Reader {
  int read(const char* const* value) { return select(value); }
};
int deep(const char* const* const* value) { return ***value == 'x'; }
int qualified(const volatile char* const*) { return 3; }
int qualified(const char* const*) { return 4; }
int main() {
  char text[] = "x";
  char* values[] = { text };
  char** pointer = values;
  const char* const* converted = pointer;
  Reader reader;
  if (select(values) != 1 || reader.read(values) != 1) return 1;
  if (deep(&pointer) != 1 || **converted != 'x') return 2;
  if (qualified(values) != 4) return 3;
  return 0;
}

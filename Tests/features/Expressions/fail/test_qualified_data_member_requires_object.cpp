// EXPECT_COMPILE_FAIL: 1
struct Record { int value; };
int main() {
  int size = sizeof(Record::value);
  if (false) return Record::value;
  return size;
}

// EXPECT_COMPILE_FAIL: 1
struct Value { int value; };
int main()
{
  const Value destination = {0};
  Value source = {1};
  destination = source;
}

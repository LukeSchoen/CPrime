// EXPECT_EXIT: 0

int main(void)
{
  int value = 0;
  int initial = 42;
  int *result = new(&value) int(initial);
  return result == &value && value == 42 ? 0 : 1;
}

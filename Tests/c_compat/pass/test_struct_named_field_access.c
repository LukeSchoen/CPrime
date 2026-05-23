// EXPECT_EXIT: 0
// EXPECT_STDOUT:
struct Example {
  int value;
};

int main(void)
{
  struct Example example;
  example.value = 1;
  return example.value == 1 ? 0 : 1;
}

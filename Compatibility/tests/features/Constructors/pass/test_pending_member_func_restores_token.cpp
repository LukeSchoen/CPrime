// EXPECT_EXIT: 0

struct PendingTokenRestore
{
  int value;

  PendingTokenRestore(int x)
  {
    value = x;
  }

  PendingTokenRestore()
  {
    value = 7;
  }
};

int main(void)
{
  PendingTokenRestore value;
  return value.value == 7 ? 0 : 1;
}

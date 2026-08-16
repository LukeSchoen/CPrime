// EXPECT_EXIT: 0

class OverloadedInit
{
public:
  int value;

  OverloadedInit(int x) : value(x) {}
  OverloadedInit() : value(4) {}
};

int main(void)
{
  OverloadedInit value;
  return value.value == 4 ? 0 : 1;
}

// EXPECT_EXIT: 0

struct Box
{
  int value;

  Box() = default;
  Box(const Box &other) = default;
  Box &operator=(const Box &other) = default;
};

int main()
{
  Box a;
  Box b;
  a.value = 7;
  b = a;
  return b.value == 7 ? 0 : 1;
}

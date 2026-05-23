// EXPECT_EXIT: 0
struct NoDtorType
{
  int v;
};

int main(void)
{
  struct NoDtorType n = {11};
  return n.v == 11 ? 0 : 1;
}

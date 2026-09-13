// EXPECT_COMPILE_FAIL: 1
struct Left {};
struct Right {};
struct Both : Left, Right {};

void select(Left &);
void select(Right &);

int main()
{
  Both value;
  select(value);
}

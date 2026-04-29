// EXPECT_EXIT: 0
// EXPECT_STDOUT:
typedef struct
{
  int x;
  int y;
} Point;

int main(void)
{
  Point p;
  p.x = 5;
  p.y = 7;
  return (p.x + p.y == 12) ? 0 : 1;
}

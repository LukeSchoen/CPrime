// EXPECT_EXIT: 0
// EXPECT_STDOUT:
typedef struct
{
  int x;
  int y;
} Point;

int sum(const Point *p)
{
  return p->x + p->y;
}

int main(void)
{
  const Point p = {8, 13};
  return p.sum() == 21 ? 0 : 1;
}

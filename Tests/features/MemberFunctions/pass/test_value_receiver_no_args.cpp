// EXPECT_EXIT: 0
// EXPECT_STDOUT:
typedef struct
{
  int x;
  int y;
} Point;

int length_sq(const Point *p)
{
  return p->x * p->x + p->y * p->y;
}

int main(void)
{
  Point p = {3, 4};
  return p.length_sq() == 25 ? 0 : 1;
}

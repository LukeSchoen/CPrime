// EXPECT_EXIT: 0
// EXPECT_STDOUT:
typedef struct
{
  int x;
  int y;
} Point;

void translate(Point *p, int dx, int dy)
{
  p->x += dx;
  p->y += dy;
}

int main(void)
{
  Point p = {1, 2};
  p.translate(4, 5);
  return (p.x == 5 && p.y == 7) ? 0 : 1;
}

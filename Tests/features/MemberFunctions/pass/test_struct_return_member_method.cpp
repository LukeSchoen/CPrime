// EXPECT_EXIT: 0
struct V3 {
  int x;
  int y;
  struct V3 plus(struct V3 o);
};

typedef struct V3 V3;

V3 V3::plus(V3 o)
{
  V3 r = { this->x + o.x, this->y + o.y };
  return r;
}

int main(void)
{
  V3 a = {1,2};
  V3 b = {3,4};
  V3 c;
  c = a.plus(b);
  return (c.x == 4 && c.y == 6) ? 0 : 1;
}

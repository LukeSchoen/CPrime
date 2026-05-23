// EXPECT_EXIT: 0
class V3 {
  int x;
  int add(int y);
};

int V3::add(int y)
{
  return this->x + y;
}

int main(void)
{
  V3 v = {9};
  V3* p = &v;
  return p->add(2) == 11 ? 0 : 1;
}

struct iRect
{
  int x;
  int y;
  int w;
  int h;
};

int area(iRect r)
{
  return r.w * r.h;
}

int main(void)
{
  iRect r = {0, 0, 5, 6};
  return area(r) == 30 ? 0 : 1;
}

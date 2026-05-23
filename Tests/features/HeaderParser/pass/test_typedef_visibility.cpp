typedef unsigned char Uint8;

typedef struct iRect
{
  int x;
  int y;
  int w;
  int h;
} iRect;

int area(iRect r)
{
  return r.w * r.h;
}

int main(void)
{
  iRect r = {0, 0, 3, 4};
  return area(r) == 12 ? 0 : 1;
}

struct Point
{
  int x = 1;
  int y = 2;
};

int main()
{
  Point p;
  return sizeof(p) == sizeof(int) * 2 ? 0 : 1;
}

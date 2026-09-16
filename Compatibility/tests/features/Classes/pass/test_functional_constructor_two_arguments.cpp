struct TwoArgPoint
{
  int x;
  int y;

  TwoArgPoint(int px, int py) : x(px), y(py) {}
};

int main()
{
  TwoArgPoint point(3, 4);
  return point.x == 3 && point.y == 4 ? 0 : 1;
}

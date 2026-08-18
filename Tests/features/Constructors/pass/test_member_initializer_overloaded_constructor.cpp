int holder_constructed;

struct Point
{
  Point(int px, int py) : x(px), y(py) {}
  Point(const Point &other) : x(other.x), y(other.y) {}
  int x;
  int y;
};

struct Holder
{
  Holder() : point(4, 6) { holder_constructed = 1; }
  Point point;
};

struct CopyHolder
{
  CopyHolder() : point(Point(7, 9)) {}
  Point point;
};

int main()
{
  Holder holder;
  CopyHolder copied;
  if (!holder_constructed)
    return 3;
  if (holder.point.x != 4 || holder.point.y != 6)
    return 1;
  return copied.point.x == 7 && copied.point.y == 9 ? 0 : 2;
}

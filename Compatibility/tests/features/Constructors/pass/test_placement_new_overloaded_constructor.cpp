struct Point
{
  Point(int px, int py) : x(px), y(py) {}
  int x;
  int y;
};

int main()
{
  char storage[sizeof(Point)];
  Point *point = new (storage) Point(4, 6);
  struct Holder { Point point; } holder;
  new (&(holder.point)) Point(7, 9);
  return point->x == 4 && point->y == 6
         && holder.point.x == 7 && holder.point.y == 9 ? 0 : 1;
}

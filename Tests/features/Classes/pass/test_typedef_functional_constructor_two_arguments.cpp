struct TypedefPointImpl
{
  int x;
  int y;

  TypedefPointImpl(int px, int py) : x(px), y(py) {}
};

typedef TypedefPointImpl TypedefPoint;

int main()
{
  TypedefPoint point(5, 6);
  return point.x == 5 && point.y == 6 ? 0 : 1;
}

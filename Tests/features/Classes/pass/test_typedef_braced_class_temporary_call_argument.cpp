struct Point { int x; int y; };
typedef Point PointAlias;

int sum(Point point) { return point.x + point.y; }

int main()
{
    return sum(PointAlias{ 4, 5 }) == 9 ? 0 : 1;
}

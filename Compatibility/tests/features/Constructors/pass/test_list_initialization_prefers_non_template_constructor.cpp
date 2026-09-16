// A braced list prefers the ordinary constructor over a member-template
// specialization that was already materialized for the same parameter types.
// The specialization is not a template definition, so its ordering could not
// separate the two candidates and the initialization was reported ambiguous.

struct Point
{
  Point() : x(0), y(0), viaTemplate(0) {}
  Point(const int &left, const int &right) : x(left), y(right), viaTemplate(0) {}
  template<typename U, typename V> explicit Point(const U &left, const V &right)
    : x((int)left), y((int)right), viaTemplate(1) {}
  int x, y, viaTemplate;
};

int main()
{
  volatile int volatileX = 3;
  volatile int volatileY = 4;
  Point fromVolatile(volatileX, volatileY);
  if (fromVolatile.viaTemplate != 1) return 1;

  Point point = Point();
  point = { 7, 8 };
  if (point.viaTemplate != 0) return 2;
  if (point.x != 7 || point.y != 8) return 3;

  Point direct{ 5, 6 };
  if (direct.viaTemplate != 0) return 4;
  if (direct.x != 5 || direct.y != 6) return 5;
  return 0;
}

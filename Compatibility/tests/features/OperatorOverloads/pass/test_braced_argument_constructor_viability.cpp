// EXPECT_COMPILE_ARGS: -Werror
int alive, copies, segment_constructors, evaluations;
struct Point {
  double x, y;
  Point(double a, double b) : x(a), y(b) { ++alive; }
  Point(const Point &other) : x(other.x), y(other.y) { ++alive; ++copies; }
  ~Point() { --alive; }
};
struct Segment {
  Point first, second;
  Segment(const Point &a, const Point &b) : first(a), second(b) {
    second.y += 10; ++segment_constructors;
  }
};
const Point &touch(const Point &point) { ++evaluations; return point; }
int select(Point point, double tolerance=0.1) { return -1; }
int select(Segment segment) { return int(segment.first.x+segment.second.y); }
struct Explicit {
  int value;
  explicit Explicit(int x, int y) : value(x+y+20) {}
};
int main() {
  {
    Point a(2,3), b(4,5);
    if(select({touch(a),touch(b)})!=17) return 1;
    if(evaluations!=2 || alive!=2 || copies!=2 || segment_constructors!=1) return 2;
    if(select({{1,2},{3,4}})!=15) return 3;
    if(alive!=2 || segment_constructors!=2) return 4;
    Point copy{a};
    if(copy.x!=2 || alive!=3) return 5;
    Explicit direct{3,4};
    if(direct.value!=27) return 6;
  }
  return alive;
}

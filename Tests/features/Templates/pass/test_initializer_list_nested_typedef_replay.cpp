// EXPECT_EXIT: 0

#include <initializer_list>

typedef unsigned char ui8;

template<typename T>
struct Vec3
{
  T x;
  T y;
  T z;
};

template<typename T>
class List
{
public:
  List() : count(0) {}
  List(const std::initializer_list<T> &values) : count((int)values.size()) {}

  int count;
};

template<typename T>
struct Box
{
  typedef Vec3<T> VertexType;

  List<VertexType> Corners() const
  {
    List<VertexType> corners =
    {
      { min.x, min.y, min.z },
      { max.x, min.y, min.z },
      { min.x, max.y, min.z },
      { max.x, max.y, max.z }
    };
    return corners;
  }

  VertexType min;
  VertexType max;
};

class Histogram
{
public:
  Histogram();
  Box<ui8> GenerateMapping(float threshold = 0.05f);
};

int main()
{
  Box<ui8> box;
  return box.Corners().count == 4 ? 0 : 1;
}

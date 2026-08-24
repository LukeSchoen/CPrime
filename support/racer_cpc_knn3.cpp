#include "clKNN3.h"

clKNN3::clKNN3(const clList<v3> &points) : m_points(points), m_knn(nullptr)
{
}

clKNN3::~clKNN3()
{
}

i32 clKNN3::Nearest(v3 point, f32 *pDistanceSquared)
{
  i32 nearest = -1;
  f32 nearestDistance = 3.402823466e+38f;
  for (i64 i = 0; i < m_points.Size(); ++i)
  {
    f32 dx = m_points[i].x - point.x;
    f32 dy = m_points[i].y - point.y;
    f32 dz = m_points[i].z - point.z;
    f32 distance = dx * dx + dy * dy + dz * dz;
    if (distance < nearestDistance)
    {
      nearestDistance = distance;
      nearest = (i32)i;
    }
  }
  if (pDistanceSquared)
    *pDistanceSquared = nearestDistance;
  return nearest;
}

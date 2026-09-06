// EXPECT_MANIFEST_SOURCE: CommonLib/CommonLib/src/Math/Geometry/clKNN3.cpp
// EXPECT_COMPILE_ARGS: -Werror
#include "nanoflann.hpp"

struct PointCloud {
  float points[32][3];
  size_t kdtree_get_point_count() const { return 32; }
  float kdtree_get_pt(size_t index, size_t dimension) const {
    return points[index][dimension];
  }
  template<class Bounds> bool kdtree_get_bbox(Bounds&) const { return false; }
};

int check_search(unsigned threads) {
  PointCloud cloud;
  for (int i = 0; i < 32; ++i) {
    cloud.points[i][0] = (float)i * 4.f;
    cloud.points[i][1] = (float)(i % 3);
    cloud.points[i][2] = (float)(i % 5);
  }
  using Metric = nanoflann::L2_Simple_Adaptor<float, PointCloud>;
  using Index = nanoflann::KDTreeSingleIndexAdaptor<Metric, PointCloud, -1, size_t>;
  nanoflann::KDTreeSingleIndexAdaptorParams options(
      4, nanoflann::KDTreeSingleIndexAdaptorFlags::None, threads);
  Index index(3, cloud, options);
  for (int queryIndex = 0; queryIndex < 32; ++queryIndex) {
    float query[3] = {cloud.points[queryIndex][0] + 0.5f,
                      cloud.points[queryIndex][1], cloud.points[queryIndex][2]};
    size_t nearest = 100;
    float distance = -1;
    nanoflann::KNNResultSet<float, size_t, size_t> result(1);
    result.init(&nearest, &distance);
    if (!index.findNeighbors(result, query)) return 1;
    if (nearest != (size_t)queryIndex || distance != 0.25f) return 2;
  }
  return 0;
}

int main() {
  int result = check_search(1);
  if (result) return result;
  return check_search(2);
}

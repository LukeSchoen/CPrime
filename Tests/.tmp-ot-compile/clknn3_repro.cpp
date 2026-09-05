#include "clKNN3.h"
#pragma warning(push, 0)
#include "KDTreeVectorOfVectorsAdaptor.h"
#pragma warning(pop)

#if defined(REPRO_CTOR) || defined(REPRO_SINGLE) || defined(REPRO_BATCH)
using KDtree = KDTreeVectorOfVectorsAdaptor<
  clList<v3>, f32, -1, nanoflann::metric_L2, size_t>;
#endif

#if defined(REPRO_CTOR)
void ReproCtor(const clList<v3>& points)
{
  KDtree* tree = clNew<KDtree>(3, points);
  clDelete(tree);
}
#endif

#if defined(REPRO_SINGLE)
i32 ReproSingle(KDtree* tree, v3 point)
{
  size_t index = 0;
  f32 distance = 0;
  nanoflann::KNNResultSet<f32, size_t, size_t> result(1);
  result.init(&index, &distance);
  tree->index->findNeighbors(result, (f32*)point.Data());
  return (i32)index;
}
#endif

#if defined(REPRO_BATCH)
clList<i32> ReproBatch(KDtree* tree, const clList<v3>& points)
{
  clList<size_t> rawIndices(points.Size(), 0);
  clList<f32> distances(points.Size(), 0);
  nanoflann::KNNResultSet<f32> result((size_t)points.Size());
  result.init(rawIndices.Data(), distances.Data());
  tree->index->findNeighbors(result, (f32*)points.Data());
  clList<i32> indices(rawIndices.Size(), 0);
  for (i32 i = 0; i < rawIndices.Size(); ++i)
    indices.PushBack((i32)rawIndices[i]);
  return indices;
}
#endif

#include <stddef.h>

template<class Value>
struct List
{
  List() {}

  Value* Data() { return data_; }

  Value data_[2];
};

template<class Distance, class Index = size_t, class Count = size_t>
class ResultSet
{
public:
  explicit ResultSet(Count capacity) : capacity_(capacity) {}

  void init(Index* indices, Distance* distances)
  {
    indices[0] = Index(capacity_);
    distances[0] = Distance(3);
  }

private:
  Count capacity_;
};

int main()
{
  List<size_t> indices;
  List<float> distances;
  ResultSet<float> result(1);
  result.init(indices.Data(), distances.Data());
  return indices.Data()[0] == 1 && distances.Data()[0] == 3.0f ? 0 : 1;
}

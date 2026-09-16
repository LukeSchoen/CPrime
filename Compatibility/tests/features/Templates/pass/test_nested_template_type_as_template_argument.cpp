template<class Value>
struct Buffer
{
  Value value;
};

namespace geometry
{
  template<class First, class Second>
  struct ResultItem
  {
    ResultItem(First first, Second second) : first(first), second(second) {}

    First first;
    Second second;
  };

  template<class Index, class Distance>
  struct Searcher
  {
    Distance Search(Buffer<ResultItem<Index, Distance>>& output,
                    const Distance& radius = {}) const
    {
      (void)output;
      return radius;
    }
  };
}

int main()
{
  geometry::Searcher<int, float> searcher;
  Buffer<geometry::ResultItem<int, float> > output;
  float radius = 2.5f;
  return searcher.Search(output, radius) == radius ? 0 : 1;
}

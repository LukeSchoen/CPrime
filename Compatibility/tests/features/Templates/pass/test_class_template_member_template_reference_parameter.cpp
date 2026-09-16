template<class Value>
struct ResultSet
{
  explicit ResultSet(Value value) : value(value) {}
  Value value;
};

struct SearchParameters
{
  SearchParameters() : sorted(true) {}
  bool sorted;
};

template<class Element>
struct Searcher
{
  using ElementType = Element;

  template<class Result>
  bool findNeighbors(Result& result, const ElementType* query,
                     const SearchParameters& parameters = {}) const
  {
    result.value = *query;
    return parameters.sorted;
  }
};

int main()
{
  float query = 6.25f;
  ResultSet<float> result(0.0f);
  Searcher<float> searcher;
  return searcher.findNeighbors(result, &query) && result.value == query
           ? 0 : 1;
}

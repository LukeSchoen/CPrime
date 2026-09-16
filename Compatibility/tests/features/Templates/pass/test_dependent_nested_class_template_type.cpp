template<class Value, class Source>
struct Result
{
  using value_type = Value;

  Result(Value initial) : value(initial) {}

  Value value;
};

struct Metric
{
  template<class Value, class Source>
  struct traits
  {
    using result_t = Result<Value, Source>;
  };
};

template<class MetricType, class Value>
struct Index
{
  using result_t =
      typename MetricType::template traits<Value, int>::result_t;
  using value_t = typename result_t::value_type;

  result_t Make(value_t value) { return result_t(value); }
};

int main()
{
  Index<Metric, float> index;
  return index.Make(4.5f).value == 4.5f ? 0 : 1;
}

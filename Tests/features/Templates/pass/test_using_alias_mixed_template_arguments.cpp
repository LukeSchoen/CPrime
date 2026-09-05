namespace metrics
{
struct L2 {};
}

template<class Container, class Scalar, int Dimension, class Metric, class Index>
struct Adaptor
{
  using self_t = Adaptor<Container, Scalar, Dimension, Metric, Index>;

  Adaptor(Scalar initial) : value(initial) {}

  self_t* Self() { return this; }

  Scalar value;
};

using Tree = Adaptor<int, float, -1, metrics::L2, unsigned>;

int main()
{
  Tree tree(6.25f);
  return tree.Self() == &tree && tree.value == 6.25f ? 0 : 1;
}

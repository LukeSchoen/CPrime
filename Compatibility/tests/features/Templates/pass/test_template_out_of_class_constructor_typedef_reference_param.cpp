template <typename T>
struct BoxPoint
{
  T x;
  T y;
};

template <typename T>
struct BoxWithTypedefCtor
{
  typedef BoxPoint<T> VertexType;

  VertexType min;
  VertexType max;

  BoxWithTypedefCtor(const VertexType &pos);
};

template <typename T>
BoxWithTypedefCtor<T>::BoxWithTypedefCtor(const VertexType &pos)
{
  min = pos;
  max = pos;
}

int main()
{
  BoxPoint<float> point = { 1.0f, 2.0f };
  BoxWithTypedefCtor<float> box(point);
  (void)box;
  return 0;
}

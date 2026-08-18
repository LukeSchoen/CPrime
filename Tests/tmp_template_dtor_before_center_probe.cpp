typedef unsigned char ui8;

template <typename T>
struct PriorList
{
  ~PriorList();
};

template <typename T>
PriorList<T>::~PriorList()
{
}

typedef PriorList<ui8> PriorBytes;

template <typename T>
struct PriorVector2
{
  T x;
  T y;

  PriorVector2() = default;
  PriorVector2(const T &_x, const T &_y) : x(_x), y(_y) {}

  template <typename U> auto operator +(const PriorVector2<U> &o) const { return PriorVector2<T>(x + o.x, y + o.y); }
  template <typename U> auto operator /(const U &val) const { return PriorVector2<T>(x / val, y / val); }
};

typedef PriorVector2<float> PriorVec2;
typedef PriorVector2<double> PriorVec2D;

template <typename T>
struct PriorBox
{
  typedef PriorVector2<T> VertexType;

  VertexType Center() const;

  VertexType min;
  VertexType max;
};

template <typename T>
PriorVector2<T> PriorBox<T>::Center() const
{
  return (min + max) / 2;
}

int main()
{
  PriorBox<float> box;
  (void)box;
  return 0;
}

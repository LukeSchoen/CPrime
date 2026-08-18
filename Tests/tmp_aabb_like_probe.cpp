typedef long long i64;

#include "clList.h"

template <typename T>
struct ProbeVector2
{
  T x;
  T y;

  ProbeVector2() = default;
  ProbeVector2(const T &_x, const T &_y) : x(_x), y(_y) {}

  ProbeVector2 operator +(const ProbeVector2 &o) const { return ProbeVector2(x + o.x, y + o.y); }
  ProbeVector2 operator -(const ProbeVector2 &o) const { return ProbeVector2(x - o.x, y - o.y); }
  ProbeVector2 operator /(const T &v) const { return ProbeVector2(x / v, y / v); }
  T &operator[](const i64 &index) { return index ? y : x; }
  const T &operator[](const i64 &index) const { return index ? y : x; }
};

template <typename T>
struct ProbeList
{
  T values[4];

  void Resize(i64 count) { (void)count; }
  T &operator[](const i64 &index) { return values[index]; }
};

template <typename T>
struct ProbeAABB2
{
  typedef ProbeVector2<T> VertexType;

  ProbeAABB2() = default;
  ProbeAABB2(const VertexType &pos);
  ProbeAABB2(const VertexType &min, const VertexType &max);
  template <typename T2> ProbeAABB2(const ProbeAABB2<T2> &box) : ProbeAABB2(VertexType(box.min), VertexType(box.max)) {}

  static ProbeAABB2 Smallest();
  static ProbeAABB2 Largest();

  T Width() const;
  T Height() const;
  T Area() const;
  VertexType Dimensions() const;
  VertexType Center() const;
  ProbeList<ProbeVector2<T>> Corners() const;
  i64 LongestEdgeAxis() const;

  union
  {
    VertexType extents[2];
    struct
    {
      VertexType min;
      VertexType max;
    };
  };

  bool operator ==(const ProbeAABB2<T> &o) const { return memcmp(this, &o, sizeof(o)) == 0; }
  bool operator !=(const ProbeAABB2<T> &o) const { return memcmp(this, &o, sizeof(o)) != 0; }
};

template <typename T>
ProbeAABB2<T>::ProbeAABB2(const VertexType &pos)
{
  min = pos;
  max = pos;
}

template <typename T>
ProbeAABB2<T>::ProbeAABB2(const VertexType &_min, const VertexType &_max)
{
  min = _min;
  max = _max;
}

template <typename T>
ProbeAABB2<T> ProbeAABB2<T>::Smallest()
{
  ProbeAABB2 ret;
  return ret;
}

template <typename T>
ProbeAABB2<T> ProbeAABB2<T>::Largest()
{
  ProbeAABB2 ret;
  return ret;
}

template <typename T>
T ProbeAABB2<T>::Width() const
{
  return max.x - min.x;
}

template <typename T>
T ProbeAABB2<T>::Height() const
{
  return max.y - min.y;
}

template <typename T>
T ProbeAABB2<T>::Area() const
{
  return Width() * Height();
}

template <typename T>
ProbeVector2<T> ProbeAABB2<T>::Dimensions() const
{
  return VertexType(max - min);
}

template <typename T>
ProbeVector2<T> ProbeAABB2<T>::Center() const
{
  return (min + max) / 2;
}

template <typename T>
i64 ProbeAABB2<T>::LongestEdgeAxis() const
{
  VertexType dim = Dimensions();
  return (dim.y > dim.x);
}

template <typename T>
ProbeList<ProbeVector2<T>> ProbeAABB2<T>::Corners() const
{
  ProbeList<VertexType> ret;
  ret.Resize(4);

  ret[0] = min;
  ret[1] = { max.x, min.y };
  ret[2] = { min.x, max.y };
  ret[3] = max;

  return ret;
}

int main()
{
  ProbeVector2<float> center;
  ProbeAABB2<float> area(center);
  return 0;
}

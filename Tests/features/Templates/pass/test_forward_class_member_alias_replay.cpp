template<class T> struct Vector {
  T value;
  Vector(T value) : value(value) {}
};
template<class T> struct Bounds;
void inspect(const Bounds<float>&);
template<class T> struct Bounds {
  typedef Vector<T> Vertex;
  Bounds(const Vertex& low, const Vertex& high);
  T sum(const Vertex& extra) const;
  Vertex low, high;
};
template<class T>
Bounds<T>::Bounds(const Vertex& low, const Vertex& high)
    : low(low), high(high) {}
template<class T> T Bounds<T>::sum(const Vertex& extra) const {
  return low.value + high.value + extra.value;
}
int main() {
  Vector<float> low(2.5f), high(4.0f);
  Bounds<float> bounds(low, high);
  return bounds.sum(low) != 9.0f;
}

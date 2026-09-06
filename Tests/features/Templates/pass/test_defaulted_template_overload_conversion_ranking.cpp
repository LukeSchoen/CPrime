template<class T> struct Ray { T value; };
template<class T> struct Box { T value; Box(T initial = T()) : value(initial) {} };
template<class A, class B> int intersects(const Ray<A>&, const Box<B>&, double* = nullptr, double* = nullptr);
template<class A, class B> int intersects(const Ray<A>&, const Box<B>&, double*, double*) { return 1; }
template<class T> struct List { T value; const T& operator[](int) const { return value; } };
template<class T> struct Tree {
  List<T> elements;
  int visit(const Ray<float>& ray, const float& tolerance) const;
};
template<class T> int Tree<T>::visit(const Ray<float>& ray, const float& tolerance) const {
  double distance;
  return intersects(ray, elements[0], &distance, tolerance);
}
template<class T> struct Triangle { T value; };
template<class A, class B> int intersects(const Ray<A>&, const Triangle<B>&, double* = nullptr, const double& = .001);
template<class A, class B> int intersects(const Ray<A>&, const Triangle<B>&, double*, const double&) { return 2; }
template<class T> int prefer(const Ray<T>&, double, int = 0) { return 3; }
template<class T> int prefer(const Ray<T>&, int) { return 4; }
int main() {
  Tree<Triangle<double>> tree;
  Ray<float> ray;
  Box<double> box;
  double enter, exit;
  if (intersects(ray, box, &enter, &exit) != 1) return 1;
  if (tree.visit(ray, .5f) != 2) return 2;
  if (prefer(ray, .5) != 3 || prefer(ray, 5) != 4) return 3;
  return 0;
}

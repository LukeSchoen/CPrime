template<typename T> struct Ray {};
template<typename T> struct Box {};
template<typename T> struct Tri {};

template<typename T1, typename T2>
bool clIntersects(const Ray<T1> &ray, const Box<T2> &box,
                  double *enter = 0, double *exit = 0);

template<typename T1, typename T2>
bool clIntersects(const Box<T1> &box, const Box<T2> &box2,
                  bool collide = false);

template<typename T1, typename T2>
bool clIntersects(const Ray<T1> &ray, const Tri<T2> &tri,
                  double *len = 0, const double &tol = 0.001);

template<typename T1, typename T2>
bool clIntersects(const Ray<T1> &ray, const Box<T2> &box,
                  double *enter, double *exit)
{
  return true;
}

template<typename T1, typename T2>
bool clIntersects(const Box<T1> &box, const Box<T2> &box2, bool collide)
{
  return collide;
}

template<typename T1, typename T2>
bool clIntersects(const Ray<T1> &ray, const Tri<T2> &tri,
                  double *len, const double &tol)
{
  return true;
}

int main()
{
  Ray<int> ray;
  Box<int> box;
  return clIntersects(ray, box, 0, 0) ? 0 : 1;
}

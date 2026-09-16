// An explicit member specialization may write a parameter whose type differs
// from the instantiated primary only by cv reached through the dependent
// array/function adjustment (CWG 1001/1322, PR c++/101402 family).
namespace cv_array_case {
template <class T>
struct A {
  void f(T);
};

static int stored;

template<class T>
void A<T>::f(const T)
{
  stored = 1;
}

template<>
void A<int[3]>::f(const int*)
{
  stored = 3;
}
}

int main()
{
  cv_array_case::A<int[3]> a;
  int values[3] = { 0, 0, 0 };
  a.f(values);
  if (cv_array_case::stored != 3)
    return 1;
  cv_array_case::A<int> b;
  b.f(0);
  return cv_array_case::stored == 1 ? 0 : 2;
}

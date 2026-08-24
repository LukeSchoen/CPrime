#include <utility>

template<typename T>
struct ForwardVector3
{
  T x, y, z;

  ForwardVector3() = default;
  ForwardVector3(const T &value);
  ForwardVector3(const T &xValue, const T &yValue,
                 const T &zValue = T(0));

  template<typename U>
  explicit ForwardVector3(const ForwardVector3<U> &other)
    : x(T(other.x)), y(T(other.y)), z(T(other.z)) {}

  template<typename U, typename V, typename W>
  explicit ForwardVector3(const U &xValue, const V &yValue, const W &zValue)
    : x(T(xValue)), y(T(yValue)), z(T(zValue)) {}
};

template<typename T>
ForwardVector3<T>::ForwardVector3(const T &value)
  : ForwardVector3(value, value, value) {}

template<typename T>
ForwardVector3<T>::ForwardVector3(const T &xValue, const T &yValue,
                                 const T &zValue)
  : x(xValue), y(yValue), z(zValue) {}

template<typename T, typename... Args>
void ForwardConstruct(T *target, Args&&... args)
{
  new(target) T(std::forward<Args>(args)...);
}

int main()
{
  ForwardVector3<float> source(64.0f, 0.0f, 0.0f);
  ForwardVector3<float> destination(1.0f);
  ForwardConstruct(&destination, source);
  return destination.x == 64.0f
      && destination.y == 0.0f
      && destination.z == 0.0f ? 0 : 1;
}

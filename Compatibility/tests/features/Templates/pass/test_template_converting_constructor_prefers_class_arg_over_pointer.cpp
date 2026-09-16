template <typename T>
struct ConvertingCtorMatrix
{
  T value;

  explicit ConvertingCtorMatrix(const T *values);

  template <typename U>
  explicit ConvertingCtorMatrix(const ConvertingCtorMatrix<U> &other)
    : value(T(other.value))
  {
  }
};

template <typename T>
ConvertingCtorMatrix<T>::ConvertingCtorMatrix(const T *values)
  : value(values[0])
{
}

typedef ConvertingCtorMatrix<float> ConvertingCtorMatrixF;
typedef ConvertingCtorMatrix<double> ConvertingCtorMatrixD;

int main()
{
  double source_values[] = { 3.0 };
  ConvertingCtorMatrixD source(source_values);
  ConvertingCtorMatrixF target(source);
  return target.value == 3.0f ? 0 : 1;
}

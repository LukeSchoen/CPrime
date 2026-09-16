typedef float ReversedOperatorF32;

template <typename T>
struct ReversedOperatorMatrix
{
  T values[2];

  ReversedOperatorMatrix(T a, T b)
  {
    values[0] = a;
    values[1] = b;
  }

  template <typename U>
  auto operator*(const U &scale) const;
};

template <typename T>
template <typename U>
auto ReversedOperatorMatrix<T>::operator*(const U &scale) const
{
  return ReversedOperatorMatrix<T>(values[0] * scale, values[1] * scale);
}

template <typename T, typename U>
ReversedOperatorMatrix<T> operator*(U scale,
                                    const ReversedOperatorMatrix<T> &matrix)
{
  return matrix * scale;
}

int main()
{
  ReversedOperatorMatrix<ReversedOperatorF32> matrix(3.0f, 5.0f);
  ReversedOperatorMatrix<ReversedOperatorF32> scaled =
    ReversedOperatorF32(2.0f) * matrix;
  return scaled.values[0] == 6.0f && scaled.values[1] == 10.0f ? 0 : 1;
}

// EXPECT_EXIT: 0

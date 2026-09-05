template<int Width> struct Matrix {
  int values[Width * Width];
  template<int Row, int Column> int &at() { return values[Row * Width + Column]; }
  template<class A, class B> int sizes() { return sizeof(A) * 10 + sizeof(B); }
};
template<class M> int access(M &matrix) {
  matrix.template at<0, 1>() = 7;
  matrix.template at<1, 0>() = 11;
  return matrix.template at<0, 1>() + matrix.template at<1, 0>();
}
int main() {
  Matrix<2> matrix;
  if (access(matrix) != 18) return 1;
  if (matrix.at<0, 1>() != 7 || matrix.at<1, 0>() != 11) return 2;
  if (matrix.sizes<char, int>() != 14) return 3;
  return matrix.sizes<char, double>() != 18;
}

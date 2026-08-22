struct Matrix
{
  double data[16];

  double &M(const int &col, const int &row)
  {
    return data[col * 4 + row];
  }
};

int main()
{
  Matrix matrix;
  matrix.M(0, 0) = 7.0;
  return matrix.data[0] == 7.0 ? 0 : 1;
}

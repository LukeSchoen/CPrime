struct Matrix
{
  int v[4];
};

const Matrix identity{ 1, 0, 0, 1 };

int main()
{
  return sizeof(identity) == sizeof(int) * 4 ? 0 : 1;
}

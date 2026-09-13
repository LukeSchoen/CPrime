/* GNU VLA pointer declarators retain their local name in C++ mode. */

int rows;
int columns;

static int stride(double *data)
{
  double (*matrix)[rows][columns] = (double (*)[rows][columns])data;
  return (int)(&(*matrix)[1][0] - data);
}

int main()
{
  double data[6];
  rows = 3;
  columns = 2;
  return stride(data) != columns;
}

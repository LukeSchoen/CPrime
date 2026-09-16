/* A cast type-id may carry a variable-length array bound (GNU extension).  The
   bound names the enclosing function's local state, so disambiguating the cast
   must defer the bound to the surrounding scope instead of requiring a
   compile-time constant. */

int main()
{
  int columns = 3;
  int data[6];
  int i;

  for (i = 0; i < 6; ++i)
    data[i] = i + 1;

  void *raw = data;
  int (*matrix)[columns] = (int (*)[columns]) raw;

  if (matrix[0][0] != 1)
    return 1;
  if (matrix[0][2] != 3)
    return 2;
  if (matrix[1][2] != 6)
    return 3;
  return 0;
}

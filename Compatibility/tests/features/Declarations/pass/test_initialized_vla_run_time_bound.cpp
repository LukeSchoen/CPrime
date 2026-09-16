/* GNU initialized VLA: the bound is not a compile-time constant, so the whole
   array is zeroed at run time before the listed elements are stored. */

int main ()
{
  int count = 3;

  {
    int values[count] = { 1 };
    if (values[0] != 1) return 1;
    if (values[1] != 0 || values[2] != 0) return 2;
  }
  {
    int values[count] = { };
    if (values[0] != 0 || values[1] != 0 || values[2] != 0) return 3;
  }
  {
    int values[count] = { 5, 6, 7 };
    if (values[0] != 5 || values[1] != 6 || values[2] != 7) return 4;
  }
  {
    char text[count] = "";
    if (text[0] != 0 || text[1] != 0 || text[2] != 0) return 5;
  }
  return 0;
}

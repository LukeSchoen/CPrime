namespace records { template<class T> struct Box { T value; }; }
int main()
{
  records::Box<int> boxes[] = {{2}, {4}};
  int total = 0;
  for (records::Box<int> &box : boxes) { box.value += 1; }
  for (const records::Box<int> &box : boxes) { total += box.value; }
  if (total != 8) return 1;
  int values[] = {3, 7};
  for (int value : values) { total += value; }
  if (total != 18) return 2;
  int *pointers[] = {&values[0], &values[1]};
  for (int *pointer : pointers) { ++*pointer; }
  if (values[0] != 4 || values[1] != 8) return 3;
  int matrix[2][2] = {{1, 2}, {3, 4}};
  for (int (&row)[2] : matrix) { row[0] += row[1]; }
  if (matrix[0][0] != 3 || matrix[1][0] != 7) return 4;
  return 0;
}

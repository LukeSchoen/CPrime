// A redeclaration may add a default argument to a parameter that an earlier
// declaration left without one; calls after that declaration use the default.
int receiver(int ii, int jj);
int first() { return receiver(3, 7); }
int receiver(int ii, int jj = 9);
int second() { return receiver(5); }
int receiver(int ii, int jj) { return ii + jj; }
int third() { return receiver(5); }
int main()
{
  return first() == 10 && second() == 14 && third() == 14 ? 0 : 1;
}

int main()
{
  int total = 0;
  {
    volatile unsigned char first[700000];
    first[0] = 3;
    first[699999] = 5;
    total += first[0] + first[699999];
  }
  {
    volatile unsigned char second[700000];
    second[0] = 7;
    second[699999] = 11;
    total += second[0] + second[699999];
  }
  return total == 26 ? 0 : 1;
}

int pick(const unsigned int *p);
int pick(const unsigned long *p);
int pick(const int *p);
int pick(const long *p);

int pick(const unsigned int *p)
{
  (void)p;
  return 1;
}

int pick(const unsigned long *p)
{
  (void)p;
  return 2;
}

int pick(const int *p)
{
  (void)p;
  return 3;
}

int pick(const long *p)
{
  (void)p;
  return 4;
}

int main()
{
  unsigned int ui = 0;
  unsigned long ul = 0;
  int si = 0;
  long sl = 0;
  return pick(&ui) == 1 && pick(&ul) == 2 && pick(&si) == 3 && pick(&sl) == 4 ? 0 : 1;
}

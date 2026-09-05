static int is_null(void *p) { return p == 0; }
static int read_value(const int *p) { return *p; }
int main(void)
{
  int value = 37;
  if (!is_null(((void *)0))) return 1;
  if (read_value((const int *)(void *)&value) != 37) return 2;
  return 0;
}

template <typename T>
struct EmptyBraceList
{
  EmptyBraceList() {}
};

struct EmptyBraceVec
{
  int x;
  int y;
};

void takes_empty_brace_default(EmptyBraceList<EmptyBraceVec> values = {})
{
  (void)values;
}

int main()
{
  takes_empty_brace_default();
  return 0;
}

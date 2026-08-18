typedef int i32;

struct ListLike
{
  int Size() const { return 3; }
};

int main()
{
  ListLike skids;
  for (i32 i = 0; i < skids.Size(); i++)
  {
  }
  return 0;
}

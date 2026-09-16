// EXPECT_EXIT: 0

struct ListLike
{
  int *data;
  int size;

  ListLike()
  {
    data = 0;
    size = 0;
  }
};

struct Owner
{
  ListLike list;

  Owner()
  {
    if (list.data != 0 || list.size != 0)
      *(int *)0 = 1;
  }
};

int main(void)
{
  Owner owner;
  return owner.list.data == 0 && owner.list.size == 0 ? 0 : 1;
}

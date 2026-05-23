// EXPECT_EXIT: 0
int ctor_ok;
int dtor_ok;

struct ListLike
{
  int *data;
  int size;

  ListLike()
  {
    data = 0;
    size = 0;
  }

  ~ListLike()
  {
    if (data == 0 && size == 0)
      dtor_ok += 1;
  }
};

struct Owner
{
  ListLike list;

  Owner()
  {
    if (list.data == 0 && list.size == 0)
      ctor_ok += 1;
  }
};

void ListLike_destructor(struct ListLike *this);

int main(void)
{
  Owner owner;
  if (owner.list.data != 0 || owner.list.size != 0)
    return 1;
  ListLike_destructor(&owner.list);
  return ctor_ok == 1 && dtor_ok == 1 ? 0 : 1;
}

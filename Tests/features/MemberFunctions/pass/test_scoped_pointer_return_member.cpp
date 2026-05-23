// EXPECT_EXIT: 0
typedef unsigned int Uint32;
#define NULL ((void *)0)

class ItemUI;

class Container
{
  public:
    ItemUI *getItem(Uint32 index);
};

ItemUI *Container::getItem(Uint32 index)
{
  return NULL;
}

int main(void)
{
  struct Container c;
  return c.getItem(0) == NULL ? 0 : 1;
}

template<int ItemSize>
class Pool
{
public:
  int Size() const
  {
    Block* current = (Block*)&block;
    return sizeof(current->items);
  }

private:
  union Item
  {
    Item* next;
    char data[ItemSize];
  };

  struct Block
  {
    Item items[4];
  };

  Block block;
};

int main()
{
  Pool<8> pool;
  return pool.Size() == 32 ? 0 : 1;
}

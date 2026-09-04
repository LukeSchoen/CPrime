template<int ItemSize>
class Pool
{
public:
  enum { ItemsPerBlock = 64 / ItemSize };

  struct Block
  {
    int items[ItemsPerBlock];
  };

  int Count() const { return sizeof(block.items) / sizeof(block.items[0]); }

private:
  Block block;
};

int main()
{
  Pool<4> pool;
  return pool.Count() == 16 ? 0 : 1;
}

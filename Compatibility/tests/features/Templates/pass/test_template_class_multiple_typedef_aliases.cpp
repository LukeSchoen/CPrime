// EXPECT_EXIT: 0

template<class T>
class AliasBox
{
public:
  typedef T value_type;
  typedef const T &reference;
  typedef unsigned long long size_type;
  typedef const T *iterator;

  iterator data;
  size_type count;

  AliasBox(iterator p, size_type n) : data(p), count(n) {}
  AliasBox() : data(0), count(0) {}

  size_type size() const { return count; }
};

int main(void)
{
  AliasBox<int> box;
  return box.size() == 0 ? 0 : 1;
}

template<typename T>
class Pool
{
public:
  class Iterator
  {
  public:
    Iterator(T value) : m_value(value) {}
    T Get() const { return m_value; }

  private:
    T m_value;
  };

  Iterator Make(T value) { return Iterator(value); }
};

int main()
{
  Pool<int> ints;
  Pool<char> chars;
  auto i = ints.Make(7);
  auto c = chars.Make(9);
  return i.Get() == 7 && c.Get() == 9 ? 0 : 1;
}

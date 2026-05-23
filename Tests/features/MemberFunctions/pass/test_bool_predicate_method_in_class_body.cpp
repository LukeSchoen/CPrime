// EXPECT_EXIT: 0

class X
{
private:
  int m_size;

public:
  bool empty()
  {
    return (this->m_size == 0);
  }
};

int main(void)
{
  X x;
  return x.empty() ? 0 : 1;
}

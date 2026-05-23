// EXPECT_EXIT: 0

template<typename T>
class M
{
public:
  void* m_data;
  unsigned long m_size;
  T* begin()
  {
    return (T*)this->m_data;
  }
};

struct B
{
  unsigned int first;
  int second;
};

int main(void)
{
  M<int> m;
  (void)m;
  return 0;
}

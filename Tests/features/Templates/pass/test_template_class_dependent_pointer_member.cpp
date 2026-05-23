// EXPECT_EXIT: 0

template<typename T>
class X
{
private:
  T* m_data;
};

int main(void)
{
  X<int> x;
  (void)x;
  return 0;
}

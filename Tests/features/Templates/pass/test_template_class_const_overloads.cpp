// EXPECT_EXIT: 0

template<typename T>
class X
{
public:
  const char* begin() const;
  const char* end() const;
};

int main(void)
{
  X<int> x;
  (void)x;
  return 0;
}

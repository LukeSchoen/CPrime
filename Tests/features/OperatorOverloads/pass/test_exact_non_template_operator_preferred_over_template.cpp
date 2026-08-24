// EXPECT_EXIT: 0
// EXPECT_STDOUT:

struct Text
{
  Text(int value) { this->marker = value; }
  int marker;
};

Text operator+(const Text &lhs, const Text &rhs)
{
  return Text(lhs.marker + rhs.marker);
}

template<typename T>
Text operator+(const Text &, const T &)
{
  return Text(99);
}

int main()
{
  Text left(3);
  Text right(4);
  Text result = left + right;
  return result.marker == 7 ? 0 : 1;
}

// EXPECT_EXIT: 0
template <typename T> class Number;
template <typename T> Number<T> operator-(const Number<T>&, const Number<T>&);

template <typename T>
class Number
{
public:
  Number(T value) : value_(value) {}
  T value() const { return value_; }
  friend Number<T> (::operator- <>)(const Number<T>&, const Number<T>&);

private:
  T value_;
};

template <typename T>
Number<T> operator-(const Number<T>& left, const Number<T>& right)
{
  return Number<T>(left.value_ - right.value_);
}

int main()
{
  Number<int> result = Number<int>(9) - Number<int>(4);
  return result.value() != 5;
}

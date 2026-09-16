template<typename T>
class ConditionalAutoVec
{
public:
  ConditionalAutoVec() : x(0) {}
  ConditionalAutoVec(T value) : x(value) {}

  auto operator-() const;

  T x;
};

template<typename T>
ConditionalAutoVec<T> MakeConditionalAutoVec(T value)
{
  return ConditionalAutoVec<T>(value);
}

template<typename T>
auto ConditionalAutoVec<T>::operator-() const
{
  return MakeConditionalAutoVec(-x);
}

ConditionalAutoVec<int> choose_conditional_auto_vec(bool flip)
{
  return flip ? -ConditionalAutoVec<int>(1) : ConditionalAutoVec<int>(1);
}

int main()
{
  return 0;
}

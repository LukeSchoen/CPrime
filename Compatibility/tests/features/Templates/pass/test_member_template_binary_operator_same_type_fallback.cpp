template <typename T>
struct TemplateOperatorVec
{
  T x;
  T y;

  template <typename U>
  auto operator-(const TemplateOperatorVec<U> &other) const
  {
    return TemplateOperatorVec<T>{ x - (T)other.x, y - (T)other.y };
  }
};

typedef TemplateOperatorVec<int> TemplateOperatorVecI;

int main()
{
  TemplateOperatorVecI lhs = { 7, 8 };
  TemplateOperatorVecI rhs = { 1, 2 };
  auto value = lhs - rhs;
  return value.x == 6 && value.y == 6 ? 0 : 1;
}

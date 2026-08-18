template <typename T>
struct TemplateOperatorVec
{
  T x;
  T y;

  template <typename U>
  auto operator-(const TemplateOperatorVec<U> &other) const;
};

typedef TemplateOperatorVec<int> TemplateOperatorVecI;

int main()
{
  TemplateOperatorVecI lhs = { 7, 8 };
  TemplateOperatorVecI rhs = { 1, 2 };
  auto value = lhs - rhs;
  (void)value;
  return 0;
}

static int assigned;

struct Value {
  int value;

  Value() : value(0) {}

  template<class T>
  void operator=(const T&) const { assigned = 1; }
};

struct TemplateCopy {
  template<int N>
  void operator=(const TemplateCopy&) { assigned = 2; }
  int value;
};

int main()
{
  const Value constant;
  Value copy;
  TemplateCopy first, second;

  copy = constant;
  if (assigned != 1 || copy.value != 0)
    return 1;
  assigned = 0;
  first.value = 3;
  second.value = 7;
  first = second;
  return assigned != 0 || first.value != 7;
}

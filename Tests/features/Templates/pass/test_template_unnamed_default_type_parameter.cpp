template<typename T, typename = T>
struct DefaultTypeParameterBox
{
  T value;
};

typedef DefaultTypeParameterBox<int> IntDefaultTypeParameterBox;

int main()
{
  IntDefaultTypeParameterBox box;
  box.value = 4;
  return box.value == 4 ? 0 : 1;
}

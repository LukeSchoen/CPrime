template <typename T>
struct InstanceEnumConstVec
{
  enum { ElementCount = 2 };
  T values[ElementCount];
};

int main()
{
  InstanceEnumConstVec<int> vec;
  int count = vec.ElementCount;
  return count != 2;
}

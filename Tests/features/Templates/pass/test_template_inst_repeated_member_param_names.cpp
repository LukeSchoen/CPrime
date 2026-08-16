template<typename T>
struct RepeatedParamNames
{
  void Push(T &&value);
  void Push(const T &value);
  void Insert(int index, T &&value);
  void Insert(int index, const T &value);
};

typedef RepeatedParamNames<int> IntRepeatedParamNames;

int main()
{
  return 0;
}

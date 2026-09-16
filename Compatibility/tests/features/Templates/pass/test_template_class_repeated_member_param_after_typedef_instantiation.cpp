#include <initializer_list>

template <typename T>
struct RepeatParamList
{
  RepeatParamList() {}
  RepeatParamList(const std::initializer_list<T> &values)
  {
    (void)values.begin();
  }
};

typedef unsigned char RepeatByte;
typedef RepeatParamList<RepeatByte> RepeatByteList;

class RepeatName
{
  RepeatParamList<char> data;
};

class RepeatParamUser
{
public:
  void first(const RepeatName &name, const RepeatParamList<float> &value);
  void second(const RepeatName &name, const RepeatParamList<float> &value);
};

int main()
{
  return 0;
}

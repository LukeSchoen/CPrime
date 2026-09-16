typedef long long i64;

template<typename T>
struct ArrayParamBox
{
  template<i64 N> ArrayParamBox(const T (&elements)[N]);
  T *data;
};

class ForwardDeclaredValue;

void UseForwardBox(const ArrayParamBox<ForwardDeclaredValue> &value);

class ForwardDeclaredValue
{
public:
  int value;
};

int main()
{
  return 0;
}

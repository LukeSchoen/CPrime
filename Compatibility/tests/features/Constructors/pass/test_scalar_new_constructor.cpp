#include <cstdlib>

class HeapValue
{
public:
  explicit HeapValue(int initial) : value(initial) {}
  int value;
};

int main()
{
  HeapValue* value = new HeapValue(42);
  int result = value->value;
  delete value;
  return result == 42 ? 0 : 1;
}

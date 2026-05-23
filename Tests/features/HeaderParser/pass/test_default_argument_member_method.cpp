#include <stddef.h>

template<typename T>
class Holder
{
public:
  T* ptr;
  void reset(T* value = NULL);
  int is_empty() { return this->ptr == NULL; }
};

template<typename T>
void Holder<T>::reset(T* value)
{
  this->ptr = value;
}

int main(void)
{
  Holder<int> h;
  h.reset();
  return h.is_empty() ? 0 : 1;
}

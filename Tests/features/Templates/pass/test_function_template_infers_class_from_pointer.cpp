template<typename T>
void destroy(T *value)
{
}

class PointerThing
{
public:
  int value;
};

int main()
{
  PointerThing value;
  destroy(&value);
  return 0;
}

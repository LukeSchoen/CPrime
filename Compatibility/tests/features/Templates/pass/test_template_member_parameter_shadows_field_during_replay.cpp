template<typename T>
struct ReplayShadow
{
  T capacity;
  T observed;

  ReplayShadow() : capacity(3), observed(0) {}

  void reserve(T capacity)
  {
    observed = capacity;
  }
};

int main()
{
  ReplayShadow<long> value;
  value.reserve(17);
  return value.capacity == 3 && value.observed == 17 ? 0 : 1;
}

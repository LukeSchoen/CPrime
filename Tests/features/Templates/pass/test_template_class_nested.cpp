// Consolidated from similar standalone regressions; each cpc_case_N preserves one.

namespace cpc_case_0
{
// EXPECT_EXIT: 0

template<typename T>
class Vec
{
public:
  T data;
};

template<typename T>
class Holder
{
private:
  Vec<T> inner;

public:
  void set(T value)
  {
    this->inner.data = value;
  }

  T get()
  {
    return this->inner.data;
  }
};

int run(void)
{
  Holder<int> h;
  h.set(9);
  return h.get() == 9 ? 0 : 1;
}
}

namespace cpc_case_1
{
// EXPECT_EXIT: 0

template<typename T>
class Box
{
public:
  T value;
};

struct Entry
{
  Box<int> second;
};

int run(void)
{
  struct Entry e;
  e.second.value = 3;
  return e.second.value == 3 ? 0 : 1;
}
}

namespace cpc_case_2
{
// EXPECT_EXIT: 0

template<typename T>
class Box
{
  typedef T Value;
  Value value;

  void set(Value next)
  {
    this->value = next;
  }

  Value get()
  {
    return this->value;
  }
};

int run(void)
{
  Box<int> box;
  Box<float> fbox;
  box.set(42);
  fbox.set(1.5f);
  if (box.get() != 42)
    return 1;
  if (fbox.get() != 1.5f)
    return 2;
  return 0;
}
}

int main()
{
  if (int code = cpc_case_0::run()) { return code; }
  if (int code = cpc_case_1::run()) { return code; }
  if (int code = cpc_case_2::run()) { return code; }
  return 0;
}

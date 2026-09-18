// Consolidated from similar standalone regressions; each cpc_case_N preserves one.

namespace cpc_case_0
{
// EXPECT_EXIT: 0

template<typename T>
class Vector
{
public:
  T value;
};

template<typename T>
class X
{
private:
  Vector<T> m_data;
};

int run(void)
{
  X<int> x;
  (void)x;
  return 0;
}
}

namespace cpc_case_1
{
// EXPECT_EXIT: 0

template<typename T>
class X
{
private:
  T data;
  T* ptr;

public:
  void set(T value)
  {
    this->data = value;
    this->ptr = &this->data;
  }

  T get()
  {
    return *this->ptr;
  }
};

int run(void)
{
  X<int> x;
  x.set(6);
  return x.get() == 6 ? 0 : 1;
}
}

namespace cpc_case_2
{
// EXPECT_EXIT: 0

template<typename T>
class X
{
private:
  T* m_data;
};

int run(void)
{
  X<int> x;
  (void)x;
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

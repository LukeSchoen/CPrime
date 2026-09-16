// EXPECT_EXIT: 0
// EXPECT_STDOUT: bound
// A reference to a base class subobject has to alias the base subobject of the
// object it is bound to, even when the base is a template specialization that
// declares its own copy/move operations.  Binding it through a derived
// reference parameter used to materialize a temporary base copy, so the
// reference, and every data pointer read through it, belonged to that copy
// instead of the caller's object.  Racer's track sampler sampled through this
// shape and faulted on the first wheel probe.
#include <stdio.h>
#include <vector>

template<typename T> class Base
{
public:
  Base() = default;
  Base(const Base &other) : m_data(other.m_data), m_size(other.m_size) { }
  Base(Base &&other) : m_data(static_cast<std::vector<T> &&>(other.m_data)), m_size(other.m_size) { }
  Base &operator=(const Base &rhs) { m_data = rhs.m_data; m_size = rhs.m_size; return *this; }
  Base &operator=(Base &&rhs) { m_data = static_cast<std::vector<T> &&>(rhs.m_data); m_size = rhs.m_size; return *this; }

  T *Data() { return m_data.data(); }
  const T *Data() const { return m_data.data(); }
  int Size() const { return (int)m_data.size(); }

protected:
  std::vector<T> m_data;
  int m_size = 0;
};

class Derived : public Base<unsigned>
{
public:
  void Init(int count, unsigned value) { m_data.assign(count, value); m_size = count; }
};

struct Sample
{
  bool aliased;
  bool sampledData;
  unsigned value;
};

static Sample Read(const Derived &derived, int index)
{
  const Base<unsigned> &base = derived;
  Sample sample;
  sample.aliased = (const void *)&base == (const void *)&derived;
  sample.sampledData = (const void *)base.Data() == (const void *)derived.Data();
  sample.value = base.Data()[index];
  return sample;
}

int main()
{
  Derived derived;
  derived.Init(16, 0x11223344u);
  Sample sample = Read(derived, 5);
  if (!sample.aliased) { printf("base reference did not bind to the subobject\n"); return 1; }
  if (!sample.sampledData) { printf("base reference does not expose the object's data\n"); return 1; }
  if (sample.value != 0x11223344u) { printf("wrong element\n"); return 1; }
  printf("bound\n");
  return 0;
}

// A pointer-to-member-function parameter pattern names the member function's
// result type, so a class template argument there deduces from that result and
// keeps its reference. The second template deduces every argument from the
// member pointer alone, which also exercises the first-parameter probe.
template <class T>
class Slot {
public:
  Slot() : value(0) {}
  void set(T v) { value = v; }
  T get() const { return value; }
private:
  T value;
};

class Host {
public:
  Slot<int>& slot() { return reference; }
  Slot<int> copy() { return reference; }
private:
  Slot<int> reference;
};

template <class T1, class T2>
T2 read_slot(T1& host, Slot<T2>& (T1::*access)())
{
  return (host.*access)().get();
}

template <class T1, class T2>
T2 read_copy(Slot<T2> (T1::*access)())
{
  T1 host;
  return (host.*access)().get();
}

int main()
{
  Host host;
  host.slot().set(7);
  if (read_slot(host, &Host::slot) != 7) return 1;
  if (read_copy(&Host::copy) != 0) return 2;
  return 0;
}

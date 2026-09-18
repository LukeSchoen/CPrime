// <memory> keeps the C++98 auto_ptr that pre-C++17 libraries still select:
// Boost's smart_ptr/scoped_ptr.hpp guards `explicit scoped_ptr(std::auto_ptr<T>)`
// with BOOST_NO_AUTO_PTR, which only a standard library Boost.Config can
// identify defines, so instantiating boost::scoped_ptr<T> (Boost.Signals2's
// foreign_ptr detail) parsed that constructor and stopped with "parameter type
// expected before 'std'".  Copying an auto_ptr transfers ownership, and the
// auto_ptr_ref proxy lets a by-value parameter take ownership from a temporary
// even though the copy constructor takes a non-const reference.
#include <memory>

static int destroyed;

struct Base
{
  virtual ~Base() { ++destroyed; }
};

struct Derived : Base
{
};

template<class T>
class holder
{
public:
  explicit holder(std::auto_ptr<T> p) : m_ptr(p.release()) {}
  ~holder() { delete m_ptr; }
  T *get() const { return m_ptr; }
private:
  holder(const holder &);
  holder &operator=(const holder &);
  T *m_ptr;
};

void consume(std::auto_ptr<Base> p)
{
  delete p.release();
}

int main()
{
  std::auto_ptr<Base> source(new Derived);
  holder<Base> owned(source);
  if (source.get() != 0 || owned.get() == 0 || destroyed != 0)
    return 1;

  std::auto_ptr<Derived> first(new Derived);
  std::auto_ptr<Derived> second(first);
  if (first.get() != 0 || second.get() == 0)
    return 2;

  std::auto_ptr<Base> base(second);
  if (second.get() != 0 || base.get() == 0)
    return 3;

  consume(base);
  if (base.get() != 0 || destroyed != 1)
    return 4;

  consume(std::auto_ptr<Derived>(new Derived));
  if (destroyed != 2)
    return 5;

  std::auto_ptr<Derived> reset(new Derived);
  reset.reset(new Derived);
  if (destroyed != 3)
    return 6;

  return 0;
}

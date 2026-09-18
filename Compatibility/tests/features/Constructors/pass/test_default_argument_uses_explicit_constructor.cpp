// A default argument is copy-initialized from its expression, but the copy
// itself is direct-initialization, so an explicit converting constructor is
// viable there ([dcl.fct.default], [class.conv.ctor]).  boost::date_time's
// `date_duration(duration_rep day_count = 0)` relies on it.
class Target
{
public:
  explicit Target(int value) : m_value(value) {}
  int value() const { return m_value; }
private:
  int m_value;
};

Target make(int value)
{
  Target result(value);
  return result;
}

int use_default(Target target = 0)
{
  return target.value();
}

class Outer
{
public:
  explicit Outer(int value = 0) : m_value(value) {}
  int value() const { return m_value; }
private:
  int m_value;
};

int use_default_outer(Outer target = 0)
{
  return target.value();
}

template <typename T>
class Boxed
{
public:
  explicit Boxed(int value = 0) : m_value(value) {}
  int value() const { return m_value; }
private:
  int m_value;
};

template <typename T>
int use_default_boxed(Boxed<T> target = 0)
{
  return target.value();
}

int main()
{
  if (use_default() != 0)
    return 1;
  if (use_default(Target(7)) != 7)
    return 2;
  if (use_default_outer() != 0)
    return 3;
  if (use_default_boxed<int>() != 0)
    return 4;
  if (make(3).value() != 3)
    return 5;
  return 0;
}

namespace compatibility
{
template<class T>
struct Underlying
{
  typedef int type;
};

enum class Mode
{
  First = 3,
  Second = 7
};

Underlying<Mode>::type Bits(Mode mode)
{
  return static_cast<Underlying<Mode>::type>(mode);
}
}

int main()
{
  return compatibility::Bits(compatibility::Mode::Second) == 7 ? 0 : 1;
}

#define TypeOf(value) decltype(value)
#define TypeInstance(type) declval<type>()
#define RequiresArg(ArgType, Arg) ArgType = Arg
#define RequiresToString(T) RequiresArg(typename, TypeOf(ToString(TypeInstance(T))))

template<typename T>
T declval();

class MacroDefaultString;
MacroDefaultString ToString(int value);

class MacroDefaultString
{
public:
  MacroDefaultString();
  MacroDefaultString(const MacroDefaultString &other);

  template<typename T, RequiresToString(T)>
  explicit MacroDefaultString(const T &value)
  {
  }
};

template<typename T, RequiresToString(T)>
MacroDefaultString operator+(const MacroDefaultString &lhs, const T &rhs)
{
  return MacroDefaultString(rhs);
}

template<typename T, RequiresToString(T)>
MacroDefaultString operator+(const T &lhs, const MacroDefaultString &rhs)
{
  return MacroDefaultString(lhs);
}

int main()
{
  return 0;
}

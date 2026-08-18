#define clTypeOf(value) decltype(value)
#define clTypeInstance(type) std::declval<type>()
#define clRequiresArg(ArgType, Arg) ArgType = Arg
#define RequiresToString(T) clRequiresArg(typename, clTypeOf(clTypeInstance(T)))

namespace std
{
  template <typename T> T declval();
}

class StringLike
{
public:
  StringLike();
  template<typename T, RequiresToString(T)> explicit StringLike(const T &text) {}
  ~StringLike() = default;
};

int main()
{
  StringLike s;
  (void)s;
  return 0;
}

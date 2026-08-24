template<typename Left, typename Right>
int combine(Left left, Right right)
{
  return (int)(left + right);
}

int consume(int value)
{
  return value;
}

template<typename T>
struct Callable
{
  int operator()(const T &value) const
  {
    return (int)value + 1;
  }
};

template<typename T>
struct Pattern
{
  T value;
};

template<typename T>
int select_pattern(const T &)
{
  static_assert(sizeof(T) == 0, "generic overload must not instantiate");
  return 0;
}

template<typename T>
int select_pattern(const Pattern<T> &value)
{
  return (int)value.value;
}

template<typename T>
struct FriendBox
{
  T value;

  template<typename U>
  friend int read_friend(FriendBox<U> *box);
};

template<typename U>
int read_friend(FriendBox<U> *box)
{
  return (int)box->value;
}

template<typename T>
struct BodyChain
{
  T value;

  BodyChain(T initial) : value(initial) {}
  ~BodyChain();
  void Clear();
};

template<typename T>
BodyChain<T>::~BodyChain()
{
  Clear();
}

template<typename T>
void BodyChain<T>::Clear()
{
  value = (T)0;
}

struct ReplayStream
{
  template<typename T>
  bool Write(const T &value);

  template<typename T>
  int Write(const T *values, int count);
};

struct FriendPublished
{
  int value;

  friend int replay_write(const FriendPublished *values, int count,
                          ReplayStream *stream);
};

template<typename T>
bool ReplayStream::Write(const T &value)
{
  return Write(&value, 1) == 1;
}

template<typename T>
int ReplayStream::Write(const T *values, int count)
{
  return replay_write(values, count, this);
}

int replay_write(const FriendPublished *values, int count,
                 ReplayStream *stream)
{
  (void)stream;
  return values && values[0].value == 19 ? count : 0;
}

template<typename T>
struct AliasHolder
{
  using ElementType = T;
};

template<typename T>
T::ElementType alias_identity(T::ElementType value)
{
  return value;
}

struct DefaultValue
{
  int value;

  DefaultValue() : value(23) {}
};

template<typename T>
struct DefaultArgument
{
  int Read(DefaultValue value = {});
};

template<typename T>
int DefaultArgument<T>::Read(DefaultValue value)
{
  return value.value;
}

template<typename T>
struct ArityOverload
{
  int Read(int value);
  int Read();
};

template<typename T>
int ArityOverload<T>::Read(int value)
{
  return value;
}

template<typename T>
int ArityOverload<T>::Read()
{
  return 29;
}

template<typename T>
int replay_local_name(T value)
{
  int result = 0;
  for (int i = 0; i < 2; ++i)
    result += (int)value;
  return result;
}

int instantiate_with_suspended_local()
{
  int i = 31;
  return replay_local_name<int>(i);
}

struct PreferConcreteMember
{
  int Add(const char &value);

  template<typename T>
  int Add(const T &value);
};

int PreferConcreteMember::Add(const char &value)
{
  return (int)value;
}

template<typename T>
int PreferConcreteMember::Add(const T &value)
{
  (void)value;
  return -1;
}

template<typename T>
struct StaticVector
{
  T value;
};

template<typename T>
struct StaticTemplateOverloads
{
  template<typename T>
  static int Select(const T &, const T &, const T &, const T &, const T &)
  {
    return 5;
  }

  template<typename T>
  static int Select(const StaticVector<T> &, const StaticVector<T> &)
  {
    return 2;
  }

  static int Run();
};

template<typename T>
int StaticTemplateOverloads<T>::Run()
{
  StaticVector<T> value;
  return Select(value, value);
}

template<typename T>
int prefer_concrete_free(T *)
{
  static_assert(false, "generic free-function fallback must not instantiate");
  return -1;
}

int prefer_concrete_free(unsigned char *value)
{
  return (int)*value;
}

template<typename T>
int call_preferred_free(T *value)
{
  return prefer_concrete_free(value);
}

class StaticSameArity
{
public:
  static int Decode(char value);
  static int Decode(int *value);
  static int DecodeTwice(char value);
};

int StaticSameArity::Decode(char value)
{
  return (int)value;
}

int StaticSameArity::Decode(int *value)
{
  return *value;
}

int StaticSameArity::DecodeTwice(char value)
{
  return Decode(value) + Decode(value);
}

int main()
{
  Pattern<int> pattern;
  FriendBox<int> friend_box;
  int combined = consume(combine<int, long>(2, 3));
  long cast_value = static_cast<long>(combined);
  ReplayStream stream;
  FriendPublished published;
  DefaultArgument<int> defaults;
  ArityOverload<int> arity;
  PreferConcreteMember preferred;
  char preferred_value = 37;
  unsigned char preferred_byte = 41;

  pattern.value = 7;
  friend_box.value = 11;
  if (Callable<int>()(combined) != 6)
    return 1;
  if (select_pattern(pattern) != 7)
    return 2;
  if (read_friend(&friend_box) != 11)
    return 3;
  if (cast_value != 5)
    return 4;
  {
    BodyChain<int> body(9);
    if (body.value != 9)
      return 5;
  }
  published.value = 19;
  if (!stream.Write(published))
    return 6;
  if (alias_identity<AliasHolder<int> >(17) != 17)
    return 7;
  if (defaults.Read() != 23)
    return 8;
  if (arity.Read(13) != 13 || arity.Read() != 29)
    return 9;
  if (instantiate_with_suspended_local() != 62)
    return 10;
  if (preferred.Add(preferred_value) != 37)
    return 11;
  if (StaticTemplateOverloads<int>::Run() != 2)
    return 12;
  if (call_preferred_free(&preferred_byte) != 41)
    return 13;
  if (StaticSameArity::DecodeTwice(7) != 14)
    return 14;
  return 0;
}

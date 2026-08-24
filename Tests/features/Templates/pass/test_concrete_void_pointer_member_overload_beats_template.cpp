struct Sink
{
  int Write(const void *data, int size);

  template<class T>
  int Write(const T *data, int count);
};

int Sink::Write(const void *data, int size)
{
  return data && size == 4;
}

template<class T>
int Sink::Write(const T *data, int count)
{
  static_assert(false, "template fallback must not be instantiated");
  return 0;
}

int main()
{
  Sink sink;
  int value = 1;
  return sink.Write((void *)&value, 4) ? 0 : 1;
}

typedef unsigned char uint8_t;
typedef unsigned long long size_t;

struct Decoder
{
  typedef uint8_t type;

  template <typename Traits>
  static typename Traits::value_type process(const uint8_t *data, size_t size,
                                             typename Traits::value_type result,
                                             Traits)
  {
    (void)data;
    (void)size;
    return Traits::low(result, 65);
  }
};

struct Counter
{
  typedef size_t value_type;

  static value_type low(value_type result, unsigned int ch)
  {
    (void)ch;
    return result + 1;
  }
};

struct Writer
{
  typedef uint8_t *value_type;

  static value_type low(value_type result, unsigned int ch)
  {
    *result = (uint8_t)ch;
    return result + 1;
  }
};

size_t explicit_call(const uint8_t *data, size_t size)
{
  return Decoder::process(data, size, (size_t)0, Counter());
}

template <typename D, typename T>
size_t convert(const char *data, size_t length, typename T::value_type dest, D, T)
{
  typename T::value_type end = D::process(
      reinterpret_cast<const typename D::type *>(data), length, dest, T());
  return end - dest;
}

int main()
{
  uint8_t data[1] = {};
  if (explicit_call(data, 1) != 1) return 1;

  uint8_t storage[1] = {};
  if (convert("a", 1, storage, Decoder(), Writer()) != 1) return 2;
  return storage[0] != 65;
}

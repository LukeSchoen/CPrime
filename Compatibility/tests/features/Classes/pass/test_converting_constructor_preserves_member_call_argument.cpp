template<typename T>
struct Buffer
{
  T *data;
  T *Data() { return data; }
};

struct Source
{
  Buffer<char> buffer;
};

struct Storage
{
  char *data;
  static char *Clobber() { return (char *)1; }
  Storage() : data(Clobber()) {}
};

struct Path
{
  Storage storage;
  const char *value;
  Path(const char *input);
};

Path::Path(const char *input) : value(input) {}

struct Image
{
  const char *value;
  Image(const Path &path) : value(path.value) {}
};

int CheckImage(const Image &image)
{
  return image.value && image.value[0] == 'o' && image.value[1] == 'k';
}

int main()
{
  Source source;
  source.buffer.data = (char *)"ok";
  return CheckImage(Image(source.buffer.Data())) ? 0 : 1;
}

// EXPECT_EXIT: 0

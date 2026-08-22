// EXPECT_EXIT: 0

typedef long long i64;

class Reader
{
public:
  template<class T> bool Read(T &destination);
  template<class T> bool Read(T *destination);
  template<class T> i64 Read(T *destination, i64 count);
  const void *Read(i64 length, i64 *bytesRead);
};

const void *Reader::Read(i64 length, i64 *bytesRead)
{
  *bytesRead = length;
  return (const void *)0;
}

int main()
{
  Reader reader;
  i64 bytes = 0;
  reader.Read(7, &bytes);
  return bytes == 7 ? 0 : 1;
}

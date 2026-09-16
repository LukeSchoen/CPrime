// EXPECT_EXIT: 0
struct Reader {
  template<int N> int number() { return N; }
  template<class T> bool read(T &value);
  template<class T> bool read(T *value);
  template<class T> int read(T *value, int count);
  int read(void *value, int length);
};
template<class T> bool Reader::read(T *value) { return read(value, 1) == 1; }
template<class T> int Reader::read(T *value, int count) { value->value = 7; return count; }
template<class T> bool Reader::read(T &value) { return read(&value); }
template<class T> struct Value { T value; };
typedef int Integer;
typedef unsigned char Byte;
template<class T> bool load(Reader *reader, T &value) { return reader->read(value); }
int main() {
  Reader reader;
  if (reader.number<3>() != 3) return 3;
  Value<Byte> first = {0};
  if (!load(&reader, first)) return 2;
  Value<Integer> value = {0};
  return load(&reader, value) && value.value == 7 && first.value == 7 ? 0 : 1;
}

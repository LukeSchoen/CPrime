struct Reader {
  template<class T> long long read(T *data, long long count);
  long long read(void *data, long long count);
};
template<class T> long long Reader::read(T *data, long long count) { return count + 1; }
long long use(Reader &reader, void *data) { return reader.read(data, 3); }
long long use_template(Reader &reader, void *data) { return reader.read<void>(data, 3); }
long long Reader::read(void *data, long long count) { return count; }
int main() { Reader reader; int value = 0; return use(reader, &value) != 3 || use_template(reader, &value) != 4 || reader.read<int>(&value, 5) != 6; }

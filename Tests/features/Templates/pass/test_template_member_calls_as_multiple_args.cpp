template<class T> struct Buffer {
  T elements[3];
  long long Size() const { return 3; }
  T *Data() { return elements; }
  const T *Data() const { return elements; }
};
static int calls, failed;
static const void *expected;
void accepts_size_data(long long size, const void *data) {
  ++calls;
  if (size != 3 || data != expected) failed = 1;
}
void call_size_data(Buffer<unsigned char> &buffer) {
  accepts_size_data(buffer.Size(), buffer.Data());
}
void call_const_size_data(const Buffer<unsigned char> &buffer) {
  accepts_size_data(buffer.Size(), buffer.Data());
}
int main() {
  Buffer<unsigned char> buffer;
  expected = buffer.elements;
  call_size_data(buffer);
  call_const_size_data(buffer);
  return failed || calls != 2;
}

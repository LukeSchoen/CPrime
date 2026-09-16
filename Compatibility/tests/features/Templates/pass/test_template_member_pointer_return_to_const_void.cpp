template<class T> struct Buffer {
  T elements[3];
  T *Data() { return elements; }
  const T *Data() const { return elements; }
};
static int calls, failed;
static const void *expected;
void accepts_void(const void *data) {
  ++calls;
  if (data != expected) failed = 1;
}
void call_data(Buffer<unsigned char> &buffer) { accepts_void(buffer.Data()); }
void call_const_data(const Buffer<unsigned char> &buffer) { accepts_void(buffer.Data()); }
int main() {
  Buffer<unsigned char> buffer;
  expected = buffer.elements;
  call_data(buffer);
  call_const_data(buffer);
  return failed || calls != 2;
}

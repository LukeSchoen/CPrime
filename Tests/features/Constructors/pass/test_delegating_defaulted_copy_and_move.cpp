#include <new>
struct FromResult {};
struct FromReference {};
struct Buffer {
  int value;
  Buffer() : value(0) {}
  Buffer(int n) : value(n) {}
  Buffer(const Buffer &o) : value(o.value) {}
  Buffer(Buffer &&o) : value(o.value) { o.value = -1; }
};
struct Value {
  Buffer buffer;
  Value(int n) : buffer(n) {}
  Value(const Value &) = default;
  Value(Value &&) = default;
  Value(FromResult);
  Value(FromReference, const Value &other) : Value(other) {}
};
Value make_value() { return Value(17); }
Value::Value(FromResult) : Value(make_value()) {}
int main() {
  union { int alignment; unsigned char bytes[sizeof(Value)]; } storage;
  for (unsigned i = 0; i < sizeof(storage.bytes); ++i) storage.bytes[i] = 0x55;
  Value *result = new(storage.bytes) Value(FromResult());
  if (result->buffer.value != 17) return 1;
  result->~Value();
  const Value source(29);
  Value copy(FromReference(), source);
  return copy.buffer.value != 29 || source.buffer.value != 29;
}

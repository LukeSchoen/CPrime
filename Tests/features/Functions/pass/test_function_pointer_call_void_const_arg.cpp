// EXPECT_COMPILE_ONLY: 1
typedef void (*BufferFn)(unsigned int, long long, const void *, unsigned int);
typedef unsigned char ui8;

extern BufferFn buffer_fn;

template <typename T>
class BufferList
{
public:
  long long Size() { return size; }
  T *Data() { return data; }

private:
  T *data;
  long long size;
};

void call_buffer(BufferList<ui8> &buffer)
{
  buffer_fn(1, buffer.Size(), buffer.Data(), 3);
}

int main()
{
  return 0;
}

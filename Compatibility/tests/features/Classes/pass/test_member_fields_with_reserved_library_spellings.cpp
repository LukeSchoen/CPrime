struct Buffer {
  int _len;
  int *_array;
  Buffer(int *data, int count) : _len(count), _array(data) {}
  int sum() const { int result = 0; for (int i = 0; i < _len; ++i) result += _array[i]; return result; }
};
int main() { int values[3] = {2, 5, 9}; Buffer b(values, 3); return b.sum() != 16; }

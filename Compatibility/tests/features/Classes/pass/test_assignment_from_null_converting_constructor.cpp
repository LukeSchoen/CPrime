struct Pointer {
  int *value;
  Pointer(int *next = 0) : value(next) {}
  Pointer &operator=(const Pointer &other) { value = other.value; return *this; }
};

int main()
{
  Pointer value((int *)1);
  value = __null;
  return value.value != 0;
}

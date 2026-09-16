struct Object {
  int value;
  Object *operator&() { return 0; }
};
int main() {
  Object object; object.value = 7;
  Object *pointer = __builtin_addressof(object);
  int array[2] = { 11, 13 };
  int (*whole)[2] = __builtin_addressof(array);
  return pointer->value != 7 || __builtin_launder(pointer)->value != 7
      || (*whole)[1] != 13 || __builtin_launder(array)[0] != 11;
}

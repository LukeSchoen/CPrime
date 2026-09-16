struct Object { int n; };
int main() {
  Object value;
  Object *Object = &value;
  Object->n = 7;
  return Object->n != 7;
}

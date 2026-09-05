static int created, destroyed, fail_constructor, fail_destructor;
struct Element {
  int id;
  Element() : id(++created) { if (id == fail_constructor) throw id; }
  ~Element() noexcept(false) {
    destroyed = destroyed * 10 + id;
    if (id == fail_destructor) throw id;
  }
};
int main() {
  Element *normal = new Element[3];
  if (created != 3 || normal[0].id != 1 || normal[2].id != 3) return 1;
  delete[] normal;
  if (destroyed != 321) return 2;
  created = destroyed = 0;
  fail_constructor = 3;
  try { new Element[4]; return 3; }
  catch (int value) { if (value != 3 || created != 3 || destroyed != 21) return 4; }
  fail_constructor = 0;
  created = destroyed = 0;
  Element *throwing = new Element[3];
  fail_destructor = 2;
  try { delete[] throwing; return 5; }
  catch (int value) { if (value != 2 || destroyed != 321) return 6; }
  fail_destructor = 0;
  created = destroyed = 0;
  delete[] new Element[0];
  if (created || destroyed) return 7;
  int *numbers = new int[4]();
  if (numbers[0] || numbers[3]) return 8;
  delete[] numbers;
  return 0;
}

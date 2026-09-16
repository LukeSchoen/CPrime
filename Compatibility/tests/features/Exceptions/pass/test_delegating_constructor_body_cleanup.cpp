static int part_destructions;
static int object_destructions;
struct Tag {};
struct Part {
  Part(int n) { if (n < 0) throw n; }
  ~Part() { ++part_destructions; }
};
struct Object {
  Part part;
  Object(int n) : part(n) {}
  Object(Tag, int n) : Object(n) { throw 42; }
  ~Object() { ++object_destructions; }
};
int main() {
  try { Object object(Tag(), 3); return 1; }
  catch (int n) { if (n != 42) return 2; }
  if (object_destructions != 1 || part_destructions != 1) return 3;
  try { Object object(Tag(), -1); return 4; }
  catch (int n) { if (n != -1) return 5; }
  return object_destructions != 1 || part_destructions != 1;
}

static int alive, destroyed;
struct Object { Object() { ++alive; } ~Object() { --alive; ++destroyed; } };
int main() {
  if (true) Object a;
  else Object b;
  if (alive || destroyed != 1) return 1;
  int i = 0;
  while (i++ < 2) Object c;
  if (alive || destroyed != 3) return 2;
  for (int n = 0; n < 2; ++n) Object d;
  return alive || destroyed != 5;
}

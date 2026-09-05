struct Early {
  int &value;
  Early(int &source) : value(source) {}
};
struct Late {
  Late(int &source) : value(source) {}
  int &value;
};
int main() {
  int source = 11;
  Early first(source);
  Late second(source);
  if (&first.value != &source || &second.value != &source) return 1;
  first.value = 19;
  if (source != 19 || second.value != 19) return 2;
  second.value = 23;
  return source != 23 || first.value != 23;
}

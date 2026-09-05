namespace {
  int value = 20;
  struct Local { int n; };
  int read() { return value; }
}
template<class T> int local_value(T v) { return v.n + 1; }
int other() { Local v = { 29 }; return read() + local_value(v); }

namespace source { namespace {
template<class T> struct Box { T value; explicit Box(T v):value(v){} };
}}
int main() {
  using source::Box;
  Box<int> b(7);
  { int Box = 3; if(Box != 3) return 1; }
  return b.value != 7;
}

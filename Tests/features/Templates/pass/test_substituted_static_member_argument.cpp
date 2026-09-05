template<int N> struct Size { static const int value = N; };
template<class T> struct Scale { static const int factor = sizeof(T); };
template<class Policy> int expanded() {
  Size<Policy::factor> direct;
  Size<Policy::factor + 3> expression;
  return direct.value + expression.value;
}
int main() { return expanded<Scale<int>>() != 11 || expanded<Scale<char>>() != 5; }

template<class T> struct Stream {
  int value;
  typedef Stream& (*Manipulator)(Stream&);
  Stream& apply(Manipulator f) { return f(*this); }
};
Stream<char>& set(Stream<char>& s) { s.value = 7; return s; }
int main() { Stream<char> s; s.apply(set); return s.value != 7; }

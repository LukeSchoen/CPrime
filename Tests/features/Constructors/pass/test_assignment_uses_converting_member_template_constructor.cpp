template<class T> struct V {
  T value;
  V(T v = T()) : value(v) {}
  template<class U> V(const V<U>& v) : value(T(v.value)) {}
};
template<class T> struct Box {
  typedef V<T> Vertex;
  Vertex min, max;
  Box() = default;
  Box(const Vertex& a, const Vertex& b) : min(a), max(b) {}
  template<class U> Box(const Box<U>& box) {
    *this = {Vertex(box.min), Vertex(box.max)};
  }
};

int live = 0, assignments = 0, constructions = 0;
struct Part {
  int value;
  Part(int v = 0) : value(v) { ++live; ++constructions; }
  Part(const Part& x) : value(x.value) { ++live; ++constructions; }
  ~Part() { --live; }
  Part& operator=(const Part& x) {
    value = x.value;
    ++assignments;
    return *this;
  }
};
template<class T> struct Record {
  Part part;
  T value;
  Record(T v) : part(int(v)), value(v) {}
  template<class U> Record(const Record<U>& other)
    : part(int(other.value)), value(T(other.value)) {}
};

int main() {
  Box<double> source(V<double>(2), V<double>(7));
  Box<float> target;
  target = source;
  if (target.min.value != 2 || target.max.value != 7) return 1;
  {
    Record<double> source(23);
    Record<float> destination(2);
    destination = source;
    if (destination.value != 23 || destination.part.value != 23
        || source.value != 23) return 2;
    if (live != 2 || assignments != 1 || constructions != 3) return 3;
  }
  return live != 0;
}

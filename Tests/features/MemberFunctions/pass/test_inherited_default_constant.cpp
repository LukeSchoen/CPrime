struct Base { typedef int State; static const State ready = 7; };
template<class T> struct Derived : Base {
  State get(State state = ready) { return state; }
};
int main() { Derived<int> d; return d.get() != 7 || d.get(3) != 3; }

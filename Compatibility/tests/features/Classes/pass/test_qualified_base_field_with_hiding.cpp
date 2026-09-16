struct Left { int padding; };
template<class T> struct Storage { T value; };
struct Derived : Left, Storage<int> {
 typedef Storage<int> Parent;
 int value;
 void set() { Parent::value = 17; value = 9; }
 int get() const { return Parent::value + value; }
 int conditional() const {
   if (Parent::value == 17) return Storage<int>::value;
   return 0;
 }
};
int main() {
 Derived d;
 d.padding = 3;
 d.set();
 return d.get() != 26 || d.padding != 3 || d.conditional() != 17;
}

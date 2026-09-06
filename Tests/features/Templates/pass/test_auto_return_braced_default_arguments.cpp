// Braced default arguments are part of the signature, before the body.
struct Value {
 int number;
 Value(int n): number(n) { }
};
template<class T> auto extract(T value = T{2}) {
 double answer = value.number;
 return answer + .5;
}
template<class T> auto combine(T first = T{3}, T second = T{4}) {
 return first.number + second.number + .25;
}
template<class T> auto declared(T value = T{5});
template<class T> auto declared(T value) { return value.number + .75; }
int main() {
 if (extract<Value>() != 2.5) return 1;
 if (combine<Value>() != 7.25) return 2;
 if (combine(Value{6}) != 10.25) return 3;
 return declared(Value{5}) != 5.75;
}


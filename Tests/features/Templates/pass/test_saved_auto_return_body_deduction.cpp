template<class T> struct Input { T values[3]; };
template<class T> auto accumulate(const T& input) {
  auto total = 0.5;
  for (int i=0;i<3;++i) total += input.values[i];
  return total;
}
template<class T> auto forward_one(const T& input) { return accumulate(input); }
template<class T> auto forward_two(const T& input) { return forward_one(input); }
template<class T> struct Result { T value; };
template<class T> auto make_result(T value) { return Result<T>{value}; }
template<class T> struct Factory {
  template<class U> auto make(U value) const {
    auto adjusted = value + 0.25;
    return make_result(adjusted);
  }
};
template<class T> auto local_result(T value) {
  struct Local { T value; };
  return Local{value};
}
template<class... T> auto from_empty(T... values) { return 6.75; }
template<class T> auto without_return(T) {}
int main() {
  Input<int> input={{1,2,3}};
  if(forward_two(input)!=6.5) return 1;
  Factory<int> first; Factory<double> second;
  if(first.make(2).value!=2.25 || second.make(4.5).value!=4.75) return 2;
  auto a=local_result(3); auto b=local_result(4.25); auto c=local_result(7);
  if(a.value!=3 || b.value!=4.25 || c.value!=7) return 3;
  if(from_empty()!=6.75) return 4;
  without_return(1);
  return 0;
}

template<class... Types> struct Tuple;
template<> struct Tuple<> {};
template<class Head, class... Tail> struct Tuple<Head,Tail...> {
  Head value;
  Tuple<Tail...> tail;
};
int main() {
  Tuple<int,double,char> values;
  values.value=7; values.tail.value=2.5; values.tail.tail.value=3;
  Tuple<> empty;
  return values.value!=7 || values.tail.value!=2.5 || values.tail.tail.value!=3
    || sizeof(empty)!=1;
}

template<class T> struct DeeplyNestedTypeWithLongDescriptiveIdentifier {
  T value;
};
typedef DeeplyNestedTypeWithLongDescriptiveIdentifier<int> A;
typedef DeeplyNestedTypeWithLongDescriptiveIdentifier<A> B;
typedef DeeplyNestedTypeWithLongDescriptiveIdentifier<B> C;
typedef DeeplyNestedTypeWithLongDescriptiveIdentifier<C> D;
typedef DeeplyNestedTypeWithLongDescriptiveIdentifier<D> E;
typedef DeeplyNestedTypeWithLongDescriptiveIdentifier<E> F;
template<class T> struct LongParameterOwner {
  template<class U> int read(const T&, const U&) { return sizeof(U); }
};
int main() {
  LongParameterOwner<F> owner;
  F nested;
  double wide = 3;
  int narrow = 7;
  return owner.read(nested, wide) != sizeof(double)
      || owner.read(nested, narrow) != sizeof(int);
}

template<class T> struct AliasBase {
  typedef int Value;
  virtual ~AliasBase() {}
};
template<class T> struct AliasDerived : AliasBase<T> {};
AliasDerived<int>::Value value = 7;
int main() { return value != 7; }

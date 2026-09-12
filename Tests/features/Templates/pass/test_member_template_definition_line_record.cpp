// EXPECT_EXIT: 0
//
// An out-of-class member-template definition is classified from its saved
// token stream.  That stream begins with a line record whose next word is the
// source line number, and the member-class scan must skip that value: reading
// it as a token derails the scan whenever the line number collides with a
// token id.  `#line 58` pins the definition onto the line whose number equals
// the token id of ':' (58), which used to abort the scan before the class
// qualifier and register the definition as a namespace-scope template, so the
// call below linked against a symbol that no input defined.

template <typename T>
struct LineRecordValue
{
  T value;

  template <typename U>
  bool operator==(const LineRecordValue<U> &other) const;
};

#line 58
template <typename T> template <typename U> bool LineRecordValue<T>::operator==(const LineRecordValue<U> &other) const { return value == other.value; }

int main()
{
  LineRecordValue<int> left = {4};
  LineRecordValue<int> right = {4};
  return left == right ? 0 : 1;
}

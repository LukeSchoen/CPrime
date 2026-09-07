// EXPECT_EXIT: 0
template<class T>
struct __attribute__((visibility("default"))) Box { T value; };
template<class T> int last(T (&values)[8192]) { return values[8191]; }
template<class T> int string_bound(T (&values)[sizeof("abcd")]);
const char *padding_noise = "unrelated token storage with different contents";
template<class T> int string_bound(T (&values)[sizeof("abcd")]) { return values[4]; }
int read(Box<int> box) { return box.value; }
int values[8192];
int main() {
  Box<int> box = {17};
  values[8191] = 23;
  int small[5] = {0, 0, 0, 0, 31};
  return read(box) != 17 || last(values) != 23 || string_bound(small) != 31;
}

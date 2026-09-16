// EXPECT_COMPILE_ARGS: -Werror
// Saved source-line payloads must not become names, keywords or parameters.
namespace search {
struct Options {
  bool sorted;
  Options(): sorted(true) {}
};
struct Result { int value; };
template<class T> struct Index {
  template<class
#line 30000
      R>
  bool find(
#line 40000
      R& result, const T* input, const Options& options = {}) const {
    result.value = *input;
    return options.sorted;
  }
  template<class R> int empty(
#line 41000
      ) const { return sizeof(R); }
};
}
int main() {
  search::Index<int> index;
  search::Result result = {0};
  int input = 37;
  if (!index.find(result, &input) || result.value != 37) return 1;
  search::Options options;
  options.sorted = false;
  if (index.find(result, &input, options)) return 2;
  if (index.empty<int>() != sizeof(int)) return 3;
  return 0;
}

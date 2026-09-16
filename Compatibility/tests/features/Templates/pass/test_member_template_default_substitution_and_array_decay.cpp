#include <type_traits>
struct InputIterator {};
namespace library {
  template<bool B, class T = void> struct Permit {};
  template<class T> struct Permit<true,T> { typedef T type; };
  template<class T> struct Range {
    int tag;
    Range(int count, const T &value) : tag(count * value) {}
    template<class InputIterator,
      class = typename Permit<!std::is_integral<InputIterator>::value>::type>
    Range(InputIterator first, InputIterator last) : tag(0) {
      for (; first != last; ++first) tag += *first;
    }
    void append(T value) { place(value); }
    template<class U> void place(U value) { tag += value; }
  };
}
using Pointer = typename std::enable_if<!std::is_integral<int*>::value,int*>::type;
int main() {
  int input[] = {2,3,5};
  library::Range<int> range(input,input+3);
  range.append(7);
  if (range.tag != 17) return 1;
  library::Range<int> filled(4,6);
  if (filled.tag != 24) return 2;
  Pointer first=input;
  library::Range<int> other(first,first+2);
  return other.tag != 5 ? 3 : 0;
}

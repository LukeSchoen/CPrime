#include <string>
#include <vector>
#include <type_traits>
struct InputIterator {
  const int *p;
  const int &operator*() const { return *p; }
  InputIterator &operator++() { ++p; return *this; }
  bool operator!=(const InputIterator &o) const { return p != o.p; }
};
static int live;
static int copies;
static int fail_copy;
struct Value {
  int n;
  explicit Value(int v = 0) : n(v) { ++live; }
  Value(const Value &o) : n(o.n) {
    if (fail_copy && ++copies == fail_copy) throw 17;
    ++live;
  }
  Value &operator=(const Value &) = delete;
  ~Value() { --live; }
};
int main() {
  std::wstring text(L"ship");
  static_assert(std::is_same<decltype(*text.begin()), wchar_t &>::value, "mutable iterator");
  text.begin()[0] = L'S';
  const std::wstring &ctext = text;
  static_assert(std::is_same<decltype(*ctext.begin()), const wchar_t &>::value, "const iterator");
  std::vector<wchar_t> command(ctext.begin(), ctext.end());
  command.push_back(L'\0');
  if (command.size() != 5 || command[0] != L'S' || command[4]) return 1;
  std::string empty;
  std::vector<char> nothing(empty.begin(), empty.end());
  if (!nothing.empty() || empty.cbegin() != empty.cend()) return 2;
  int input[] = {3, 5, 8};
  std::vector<int> single(input, input + 3);
  std::vector<int> count(3, 9);
  if (single.size() != 3 || single[2] != 8 || count[2] != 9) return 3;
  {
    Value input_values[] = {Value(4), Value(7)};
    {
      std::vector<Value> values(input_values, input_values + 2);
      values.reserve(8);
      values.push_back(values[0]);
      values.resize(6);
      if (values.size() != 6 || values[2].n != 4 || values[5].n != 0) return 4;
      values.resize(1);
      std::vector<Value> copied(values);
      if (live != 4 || copied[0].n != 4) return 5;
      fail_copy = 2; copies = 0;
      try { std::vector<Value> failed(input_values, input_values + 2); return 6; }
      catch (int error) { if (error != 17 || live != 4) return 7; }
      fail_copy = 0;
    }
    if (live != 2) return 8;
  }
  return live != 0 ? 9 : 0;
}
